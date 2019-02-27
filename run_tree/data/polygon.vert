#version 450

layout(push_constant) uniform PushConstants {
	vec2 position;
	uint edge_count;
	float size;
	vec4 color;

	#if 0
	bool use_uv;
	vec2 uv;
	#endif
} push_info;

layout(location = 0) out uint edge_count;
layout(location = 1) out float size;
layout(location = 2) out vec4 color;

#if 0
layout(location = 3) out vec2 uv;
#endif

void main () {
	#if 0
	gl_Position = vec4(position, (push_info.use_uv ? 1.0 : 0.0), 1.0);
	#else
	gl_Position = vec4(position, 0.0, 1.0);
	#endif

	edge_count = push_info.edge_count;
	size = push_info.size;
	color = push_info.color;

	#if 0
	uv = push_info.uv;
	#endif
}
