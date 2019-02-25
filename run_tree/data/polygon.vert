#version 450

layout(push_constant) uniform pushConstants {
	vec2 position;
	vec2 dimensions;
	float rotation;
	vec4 color;
	int edge_count;
} push_info;

layout(location = 0) out vec4  color_or_texture;
layout(location = 1) out vec4 rotation_dimensions;
layout(location = 2) out ivec4 count_type_flags;

void main () {
	color_or_texture	= push_info.color;
	rotation_dimensions = vec4(push_info.rotation, push_info.dimensions, 0.0);
	count_type_flags	= ivec4(push_info.edge_count, 0, 0, 0); 
	gl_Position = vec4(push_info.position, 0.0, 0.0);
}

// D:\VulkanSDK\1.1.92.1\bin\glslangValidator.exe -V -o run_tree/polygon_g.spv run_tree\polygon.geom 
