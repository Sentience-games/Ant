#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec4 in_color[];
layout(location = 1) in vec4 in_viewport_uv[];

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_uv;

void main ()
{
	vec2 position   = gl_in[0].gl_Position.xy;
	vec2 dimensions = gl_in[0].gl_Position.zw;
	vec2 viewport   = in_viewport_uv[0].xy;
	vec2 uv_base	= in_viewport_uv[0].zw;

	mat2 viewport_matrix = mat2(1.0 / viewport.x, 0.0,
								0.0, 1.0 / viewport.y);

	vec2 upper_left  = vec2(0.0, 0.0);
	vec2 upper_right = vec2(1.0, 0.0);
	vec2 lower_left  = vec2(0.0, 1.0);
	vec2 lower_right = vec2(1.0, 1.0);

	gl_Position = vec4(2.0 * (viewport_matrix * (position + (upper_right * dimensions))) - vec2(1.0, 1.0), 0.0, 1.0);
	out_color	= in_color[0];
	out_uv		= uv_base + upper_right;
	EmitVertex();

	gl_Position = vec4(2.0 * (viewport_matrix * (position + (upper_left * dimensions)))  - vec2(1.0, 1.0), 0.0, 1.0);
	out_color	= in_color[0];
	out_uv		= uv_base + upper_left;
	EmitVertex();

	gl_Position = vec4(2.0 * (viewport_matrix * (position + (lower_left * dimensions)))  - vec2(1.0, 1.0), 0.0, 1.0);
	out_color	= in_color[0];
	out_uv		= uv_base + lower_left;
	EmitVertex();

	EndPrimitive();

	gl_Position = vec4(2.0 * (viewport_matrix * (position + (upper_right * dimensions))) - vec2(1.0, 1.0), 0.0, 1.0);
	out_color	= in_color[0];
	out_uv		= uv_base + upper_right;
	EmitVertex();

	gl_Position = vec4(2.0 * (viewport_matrix * (position + (lower_left * dimensions)))  - vec2(1.0, 1.0), 0.0, 1.0);
	out_color	= in_color[0];
	out_uv		= uv_base + lower_left;
	EmitVertex();

	gl_Position = vec4(2.0 * (viewport_matrix * (position + (lower_right * dimensions))) - vec2(1.0, 1.0), 0.0, 1.0);
	out_color	= in_color[0];
	out_uv		= uv_base + lower_right;
	EmitVertex();

	EndPrimitive();
}
