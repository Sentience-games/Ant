#pragma once

#include "ant_shared.h"
#include "math/vector.h"

enum IMMEDIATE_TYPE
{
	IMMEDIATE_POLYGON,
	IMMEDIATE_TEXT
};

enum IMMEDIATE_LAYERS
{
	ImmediateLayer_Bottom,
	ImmediateLayer_UI_Background,
	ImmediateLayer_UI_Foreground,
	ImmediateLayer_Top,

	IMMEDIATE_LAYER_COUNT
};

struct immediate_render_request
{
	enum8(IMMEDIATE_TYPE) type;
	immediate_render_request* next;

	u32 layer;

	v2 position;
	v4 color;

	union
	{
		/// POLYGON ///
		struct
		{
			v2 dimensions;
			f32 size;
			u32 edge_count;
			f32 rotation;
		};

		/// TEXT ///
		struct
		{
			u32 font_id;
			u32 text_size;
			string text;
		};
	};
};

struct immediate_polygon_push_info
{
	v4 position_size_ec;
	v4 color;
	v4 dimensions_viewport;
	f32 rotation;
};

struct immediate_text_push_info
{
	v4 position_dimensions;
	v4 color;
	v2 uv;
};
