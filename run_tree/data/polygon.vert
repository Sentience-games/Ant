#version 450

layout(push_constant) uniform pushConstants {
	vec2 position;
	float width;
	float height;
	uint edge_count;
	float size;
	float offset;
    vec4 color;
	float rotation;
} push_info;

layout(binding = 0) uniform TextCoords
{
	vec2 uv[64];
} uv_data;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 ec_size_offset;
layout(location = 2) out float rotation;
layout(location = 3) out vec2 out_uv[64];

void main () {
	gl_Position = vec4(push_info.position, push_info.width, push_info.height);
	color = push_info.color;
	ec_size_offset = vec4(float(push_info.edge_count), push_info.size, push_info.offset, 0.0);
	rotation = push_info.rotation;
    out_uv = uv_data.uv;
}

// D:\VulkanSDK\1.1.92.1\bin\glslangValidator.exe -V -o run_tree/polygon_g.spv run_tree\polygon.geom 
