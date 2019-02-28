#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 128) out;

layout(location = 0) in vec4  in_color[];
layout(location = 1) in vec4  in_dimensions[];
layout(location = 2) in float in_rotation[];

layout(location = 0) out vec4 out_color;

const float PI = 3.1415926535;

mat2 GetRotation (float angle)
{
	return mat2(cos(-angle), sin(-angle),
				-sin(-angle), cos(-angle));
}

mat2 GetScale (float width, float height)
{
	return mat2(width, 0,
				0, height);
}

void main ()
{
	vec2 position			 = gl_in[0].gl_Position.xy;
	vec2 dimensions			 = in_dimensions[0].xy;
	vec2 viewport_dimensions = in_dimensions[0].zw;
	uint edge_count			 = uint(gl_in[0].gl_Position.w);
	float radius			 = gl_in[0].gl_Position.z;
	float rotation			 = in_rotation[0];

	float walk_angle = (2 * PI) / edge_count;

	vec2 first_vertex	 = vec2(radius, 0.0);

	if (edge_count == 3)
	{
		first_vertex = GetRotation(-walk_angle / 4.0) * first_vertex;
	}

	else
	{
		first_vertex = GetRotation(-walk_angle / 2.0) * first_vertex;
	}

	vec2 previous_vertex = GetRotation(walk_angle) * first_vertex;

	mat2 correction_matrix = GetRotation(rotation) * GetScale(dimensions.x, dimensions.y);
	mat2 viewport_matrix   = GetScale(1.0 / viewport_dimensions.x, 1.0 / viewport_dimensions.y);

	vec2 first_vertex_adjusted	  = viewport_matrix * (position + correction_matrix * first_vertex);
	vec2 previous_vertex_adjusted = viewport_matrix * (position + correction_matrix * previous_vertex);
	
	for (uint i = 0; i < edge_count; ++i)
	{
		gl_Position = vec4(first_vertex_adjusted, 0.0, 1.0);
		out_color	= in_color[0];
		EmitVertex();

		gl_Position = vec4(previous_vertex_adjusted, 0.0, 1.0);
		out_color	= in_color[0];
		EmitVertex();

		previous_vertex			 = GetRotation(i * walk_angle) * first_vertex;
		previous_vertex_adjusted = viewport_matrix * (position + correction_matrix * previous_vertex);

		gl_Position = vec4(previous_vertex_adjusted, 0.0, 1.0);
		out_color	= in_color[0];

		EmitVertex();

		EndPrimitive();
	}
}
