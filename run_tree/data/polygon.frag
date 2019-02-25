#version 450

layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_texture_coord;

layout(location = 0) out vec4 out_color;

void main () {
	out_color = frag_color;
}

// D:\VulkanSDK\1.1.92.1\bin\glslangValidator.exe -V -o run_tree/polygon_g.spv run_tree\polygon.geom 
