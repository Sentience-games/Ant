#version 450

layout(push_constant) uniform PushConstants {
	vec4  position_size_ec;
	vec4  color;
	vec4  dimensions_viewport;
	vec4 rotation_uv;
} push_info;

layout(location = 0) out vec4  out_color;
layout(location = 1) out vec4  out_dimensions;
layout(location = 2) out vec4  out_rotation_uv;

void main () {
	gl_Position = push_info.position_size_ec;

	out_color		= push_info.color;
	out_dimensions  = push_info.dimensions_viewport;
	out_rotation_uv	= push_info.rotation_uv;
}
