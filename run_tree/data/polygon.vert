#version 450

layout(push_constant) uniform PushConstants {
	vec4  position_size_ec;
	vec4  color;
	vec4  dimensions_viewport;
	float rotation;
} push_info;

layout(location = 0) out vec4  color;
layout(location = 1) out vec4  dimensions;
layout(location = 2) out float  rotation;

void main () {
	gl_Position = push_info.position_size_ec;

	color		= push_info.color;
	dimensions  = push_info.dimensions_viewport;
	rotation	= push_info.rotation;
}
