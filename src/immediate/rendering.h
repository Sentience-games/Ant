#pragma once

global_variable
struct {
	immediate_render_request* first;
	immediate_render_request* current;
} ImmediateRenderRequestQueue;

inline void
PushImmediatePolygon(v2 position, u32 edge_count, v2 dimensions, f32 size, v4 color, f32 rotation = 0.0f)
{
	immediate_render_request* next_request = PushStruct(FrameTempArena, immediate_render_request);

	next_request->type		 = IMMEDIATE_POLYGON;
	next_request->position   = position;
	next_request->color	     = color;
	next_request->dimensions = dimensions;
	next_request->size		 = size;
	next_request->edge_count = edge_count;
	next_request->rotation	 = rotation;

	next_request->next = NULL;

	if (!ImmediateRenderRequestQueue.first)
	{
		ImmediateRenderRequestQueue.first	= next_request;
		ImmediateRenderRequestQueue.current = next_request;
	}

	else
	{
		ImmediateRenderRequestQueue.current->next = next_request;
		ImmediateRenderRequestQueue.current		  = next_request;
	}
}

inline void
PushIMTri(v2 position, v2 dimensions, v4 color, f32 rotation = 0.0f)
{
	PushImmediatePolygon(position, 3, dimensions, 1.0f, color, rotation);
}

inline void
PushIMQuad(v2 position, v2 dimensions, v4 color, f32 rotation = 0.0f)
{
	PushImmediatePolygon(position, 4, dimensions, 1.0f, color, rotation);
}

inline void
PushIMCircle(v2 position, f32 radius, v4 color)
{
	PushImmediatePolygon(position, 42, {1.0f, 1.0f}, radius, color);
}

inline void
RenderImmediateCalls (VkCommandBuffer command_buffer)
{
	immediate_render_request* next_request = ImmediateRenderRequestQueue.first;
	immediate_render_request* prev_request = NULL;

	while(next_request)
	{
		if (next_request->type == IMMEDIATE_POLYGON)
		{
			if (!(prev_request && prev_request->type == IMMEDIATE_POLYGON))
			{
				VulkanAPI->vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
											 VulkanState->render_target.immediate_pipeline);
			}

			immediate_push_info push_info = {};
			push_info.position_size_ec = {next_request->position.x, next_request->position.y, next_request->size, (f32) next_request->edge_count};
			push_info.color			   = next_request->color;

			push_info.dimensions_viewport = {next_request->dimensions.x, next_request->dimensions.y,
											 VulkanState->render_target.immediate_viewport_dimensions.x,
											 VulkanState->render_target.immediate_viewport_dimensions.y};
			
			push_info.rotation = next_request->rotation;

			VulkanAPI->vkCmdPushConstants(command_buffer, VulkanState->render_target.immediate_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
										  sizeof(immediate_push_info), &push_info);

			VulkanAPI->vkCmdDraw(command_buffer, 1, 1, 0, 0);
		}

		else
		{
			INVALID_CODE_PATH;
		}

		next_request = next_request->next;
	}

	ImmediateRenderRequestQueue.first   = NULL;
	ImmediateRenderRequestQueue.current = NULL;
};
