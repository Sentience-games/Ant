#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

//
layout(location = 0) in vec4  color_or_texture[];
layout(location = 1) in vec4 rotation_dimensions[];

// NOTE(soimn): type and flags are currently unused, but are planned to hold info about textures and ui element type
layout(location = 2) in ivec4 count_type_flags[];

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_texture_coord;

void main () {
	vec2 root_pos	  = gl_in[0].gl_Position.xy;
	vec2 dimensions	  = rotation_dimensions[0].yz;
	vec4 color		  = color_or_texture[0];
	vec2 tex_coord	  = color_or_texture[0].xy;
	uint vertex_count = count_type_flags[0].x;

	vec2 previous_vertex = root_pos;

	if (vertex_count == 4)
	{
		vec2 UPLE = vec2(-1.0, 1.0);
		vec2 UPRI = vec2(dimensions.x, 1.0);
		vec2 LOLE = vec2(-1.0, -dimensions.y);
		vec2 LORI = vec2(dimensions.x, dimensions.y);

		UPLE += root_pos;
		UPRI += root_pos;
		LOLE += root_pos;
		LORI += root_pos;

		gl_Position = vec4(UPRI, 0.0 , 0.0);
		frag_color = color;
		EmitVertex();

		gl_Position = vec4(UPLE, 0.0 , 0.0);
		frag_color = color;
		EmitVertex();

		gl_Position = vec4(LOLE, 0.0 , 0.0);
		frag_color = color;
		EmitVertex();

		EndPrimitive();

		gl_Position = vec4(LOLE, 0.0 , 0.0);
		frag_color = color;
		EmitVertex();

		gl_Position = vec4(LORI, 0.0 , 0.0);
		frag_color = color;
		EmitVertex();

		gl_Position = vec4(UPRI, 0.0 , 0.0);
		frag_color = color;
		EmitVertex();
	
		EndPrimitive();
	}
}

// D:\VulkanSDK\1.1.92.1\bin\glslangValidator.exe -V -o run_tree/polygon_g.spv run_tree\polygon.geom 
