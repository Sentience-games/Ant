#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec4 color[];
layout(location = 1) in vec4 ec_size_offset[];
layout(location = 2) in float rotation[];
layout(location = 3) in vec2 uv[][64];

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_texture_coord;

mat2 GetRotation(float rads)
{
	mat2 result;
	vec2 col_1 = vec2(cos(rads), sin(rads));
	vec2 col_2 = vec2(-sin(rads), cos(rads));
	result[0] = col_1;
	result[1] = col_2;

	return result;
}

mat2 GetScale(float width, float height)
{
	mat2 result;
	vec2 col_1 = vec2(width, 0);
	vec2 col_2 = vec2(0, height);
	result[0] = col_1;
	result[1] = col_2;

	return result;
}
    
const float PI = 3.1415926535;

void main () {
	vec2 first_vertex;
	vec2 previous_vertex;
	uint edge_count = uint(ec_size_offset[0].x);
	float size = ec_size_offset[0].y;
	float offset = ec_size_offset[0].z;
    float width = gl_in[0].gl_Position.z;
    float height = gl_in[0].gl_Position.w;
	mat2 adjustment = GetRotation(rotation[0]) * GetScale(width, height) * GetRotation(offset);

	if (edge_count > 64)
	{
		edge_count = 64;
	}

	float central_angle = (2.0 * PI) / float(edge_count);
	float current_angle = 0.0;

	float radius = size;

	first_vertex = adjustment * vec2(radius, 0);
	gl_Position = vec4(first_vertex, 0.0, 0.0);
	frag_color = color[0];
    frag_texture_coord = uv[0][0];
	EmitVertex();
	previous_vertex = first_vertex;
	current_angle += central_angle;

	previous_vertex = adjustment * GetRotation(current_angle) * previous_vertex;
	gl_Position = vec4(previous_vertex, 0.0, 0.0);
	frag_color = color[0];
    frag_texture_coord = uv[0][1];
	EmitVertex();
	current_angle += central_angle;

	previous_vertex = adjustment * GetRotation(current_angle) * previous_vertex;
	gl_Position = vec4(previous_vertex, 0.0, 0.0);
	frag_color = color[0];
    frag_texture_coord = uv[0][2];
	EmitVertex();
	current_angle += central_angle;

	EndPrimitive();

    uint current_uv_index = 3;
	while (current_angle < (2 * PI - central_angle))
	{
		gl_Position = vec4(first_vertex, 0.0, 0.0);
		frag_color = color[0];
        frag_texture_coord = uv[0][0];
		EmitVertex();

		gl_Position = vec4(previous_vertex, 0.0, 0.0);
		frag_color = color[0];
        frag_texture_coord = uv[0][current_uv_index];
		EmitVertex();
		current_angle += central_angle;
        ++current_uv_index;

		previous_vertex = adjustment * GetRotation(current_angle) * previous_vertex;
		gl_Position = vec4(previous_vertex, 0.0, 0.0);
		frag_color = color[0];
        frag_texture_coord = uv[0][current_uv_index];
		EmitVertex();

		EndPrimitive();
	}
}

// D:\VulkanSDK\1.1.92.1\bin\glslangValidator.exe -V -o run_tree/polygon_g.spv run_tree\polygon.geom 
