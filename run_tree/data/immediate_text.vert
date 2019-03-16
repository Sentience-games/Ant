#version 450

layout(push_constant)  uniform PushContstants
{
	vec4 position_dimensions;
	vec4 color;
	vec4 viewport_uv;
} push_info;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_viewport_uv;

void main ()
{
	gl_Position		= push_info.position_dimensions;

	out_color		= push_info.color;
	out_viewport_uv = push_info.viewport_uv;
}
