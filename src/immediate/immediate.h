#pragma once

#include "ant_shared.h"
#include "math/vector.h"
#include "vulkan.h"

enum IMMEDIATE_TYPE
{
	IMMEDIATE_POLYGON,
	IMMEDIATE_TEXT
};

struct immediate_render_request
{
	enum8(IMMEDIATE_TYPE) type;
	immediate_render_request* next;

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
			u32 text_size;
			u32 string_length;
		};
	};
};

struct immediate_push_info
{
	v4 position_size_ec;
	v4 color;
	v4 dimensions_viewport;
	f32 rotation;
};
