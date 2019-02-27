#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 3)

layout(location = 0) in uint in_edge_count[];
layout(location = 1) in uint in_size[];
layout(location = 2) in vec4 in_color[];

#if 0
layout(location = 3) in vec2 in_uv[];
#endif

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec3 out_uv;

const float PI = 3.1415926535;

void main ()
{
	vec2 position = gl_in[0].gl_Position.xy;
	uint edge_count = in_edge_count[0];

	float central_angle = (2 * PI) / edge_count;
	mat2 walk_matrix =  mat2(cos(central_angle), sin(central_angle),
							-sin(central_angle), cos(central_angle));
	
	float radius = in_size[0];

	vec4 first_vertex = vec4(radius, 0.0, 0.0, 1.0);
	vec4 previous_vertex = walk_matrix * first_vertex;
	
	#if 0
	use_uv_flag = gl_in[0].gl_Position.z;

	vec2 first_uv = vec4(in_uv[0], 0.0, 0.0) + vec4(radius, 0.0);
	vec2 prev_uv = first_uv;
	#endif

	for (uint i = 0; i < edge_count; ++i)
	{
		gl_Position = first_vertex;
		out_color = in_color[0];
		#if 0
		out_uv = vec3(first_uv, use_uv_flag);
		#endif
		EmitVertex();

		gl_Position = previous_vertex;
		out_color = in_color[0];
		#if 0
		out_uv = vec3(prev_uv, use_uv_flag);
		#endif
		EmitVertex();

		previous_vertex = walk_matrix * previous_vertex;
		gl_Position = previous_vertex;
		out_color = in_color[0];
		#if 0
		prev_uv = walk_matrix * prev_uv;
		out_uv = vec3(prev_uv, use_uv_flag);
		#endif
		EmitVertex();

		EndPrimitive();
	}
}
