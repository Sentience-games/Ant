#pragma once

global_variable
struct {
	immediate_render_request* first[IMMEDIATE_LAYER_COUNT];
	immediate_render_request* current[IMMEDIATE_LAYER_COUNT];
} ImmediateRenderRequestQueue;

inline void
AddImmediateRenderRequest_(immediate_render_request* request, enum32(IMMEDIATE_LAYER) layer)
{
	request->next = NULL;

	if (!ImmediateRenderRequestQueue.first[layer])
	{
		ImmediateRenderRequestQueue.first[layer]	= request;
		ImmediateRenderRequestQueue.current[layer]  = request;
	}

	else
	{
		ImmediateRenderRequestQueue.current[layer]->next  = request;
		ImmediateRenderRequestQueue.current[layer]		  = request;
	}
}

inline void
PushImmediatePolygon(v2 position, u32 edge_count, v2 dimensions, f32 size, v4 color,
					 enum32(IMMEDIATE_LAYER) layer = ImmediateLayer_Top, f32 rotation = 0.0f)
{
	immediate_render_request* next_request = PushStruct(FrameTempArena, immediate_render_request);

	next_request->type		 = IMMEDIATE_POLYGON;
	next_request->layer		 = layer;
	next_request->position   = position;
	next_request->color	     = color;
	next_request->dimensions = dimensions;
	next_request->size		 = size;
	next_request->edge_count = edge_count;
	next_request->rotation	 = rotation;

	AddImmediateRenderRequest_(next_request, layer);
}

inline void
PushIMText(v2 position, v4 color, u32 font_id, u32 text_size, enum32(IMMEDIATE_LAYER) layer, const char* text, u32 text_length = 0)
{
	if (text_length == 0)
	{
		text_length = strlength(text);
	}

	if (text_length > 0)
	{
		immediate_render_request* next_request = PushStruct(FrameTempArena, immediate_render_request);
		u8* text_data = (u8*) PushString(FrameTempArena, (char*) text, text_length);

		string str = {(memory_index) text_length, text_data};

		next_request->type		 = IMMEDIATE_TEXT;
		next_request->layer		 = layer;
		next_request->position	 = position;
		next_request->color		 = color;
		next_request->font_id	 = font_id;
		next_request->text_size  = text_size;
		next_request->text		 = str;

		AddImmediateRenderRequest_(next_request, layer);
	}

	else
	{
		LOG_INFO("Immediate Renderer Warning: PushIMText was called with an empty string");
	}
}

inline void
PushIMText(v2 position, v4 color, u32 font_id, u32 text_size, enum32(IMMEDIATE_LAYER) layer, string text)
{
	// NOTE(soimn): this cast: (u32) text_size, assumes that no sane person will try to output 4 billion characters at a time
	PushIMText(position, color, font_id, text_size, layer, (char*) text.data, (u32) text.size);
}

inline void
PushIMTri(v2 position, v2 dimensions, v4 color, enum32(IMMEDIATE_LAYER) layer = ImmediateLayer_Top, f32 rotation = 0.0f)
{
	PushImmediatePolygon(position, 3, dimensions, 1.0f, color, layer, rotation);
}

inline void
PushIMQuad(v2 position, v2 dimensions, v4 color, enum32(IMMEDIATE_LAYER) layer = ImmediateLayer_Top, f32 rotation = 0.0f)
{
	PushImmediatePolygon(position, 4, dimensions, 1.0f, color, layer, rotation);
}

inline void
PushIMCircle(v2 position, f32 radius, v4 color, enum32(IMMEDIATE_LAYER) layer = ImmediateLayer_Top)
{
	PushImmediatePolygon(position, 42, {1.0f, 1.0f}, radius, color, layer);
}

inline void
RenderImmediateCalls (VkCommandBuffer command_buffer)
{
	for (u32 i = 0; i < IMMEDIATE_LAYER_COUNT; ++i)
	{
		immediate_render_request* next_request = ImmediateRenderRequestQueue.first[i];
		immediate_render_request* prev_request = NULL;

		while(next_request)
		{
			if (next_request->type == IMMEDIATE_POLYGON)
			{
				if (!(prev_request && prev_request->type == IMMEDIATE_POLYGON))
				{
					Vulkan->vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
												 Vulkan->render_target.immediate_pipeline);
				}

				immediate_polygon_push_info push_info = {};
				push_info.position_size_ec = {next_request->position.x, next_request->position.y, next_request->size, (f32) next_request->edge_count};
				push_info.color			   = next_request->color;

				push_info.dimensions_viewport = {next_request->dimensions.x, next_request->dimensions.y,
												 Vulkan->render_target.immediate_viewport_dimensions.x,
												 Vulkan->render_target.immediate_viewport_dimensions.y};
				
				push_info.rotation = next_request->rotation;

				Vulkan->vkCmdPushConstants(command_buffer, Vulkan->render_target.immediate_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
											  sizeof(immediate_polygon_push_info), &push_info);

				Vulkan->vkCmdDraw(command_buffer, 1, 1, 0, 0);
			}

			else if (next_request->type == IMMEDIATE_TEXT)
			{
				if (!(prev_request && prev_request->type == IMMEDIATE_TEXT))
				{
				
				}

				immediate_text_push_info push_info = {};
				push_info.color = next_request->color;

				v2 at = next_request->position;
				
				for (u32 j = 0; j < next_request->text.size; ++j)
				{
// 					font_glyph_info glyph_info = GetCodepointGlyphInfo(next_request->font_id, (char) next_request->text.data[j]);
					font_glyph_info glyph_info = {};
					
					push_info.position_dimensions.xy = at + Vec2(-glyph_info.bearing_x, glyph_info.ascent);
					push_info.position_dimensions.zw = glyph_info.dimensions.zw;
					push_info.uv					 = glyph_info.dimensions.xy;
				}
			}

			else
			{
				INVALID_CODE_PATH;
			}

			next_request = next_request->next;
		}

		ImmediateRenderRequestQueue.first[i]   = NULL;
		ImmediateRenderRequestQueue.current[i] = NULL;
	}
};
