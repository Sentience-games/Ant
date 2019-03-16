#version 450

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_uv;

layout(location = 0) out vec4 out_color;

// TODO(soimn): signed distance field font rendering

void main () {
	bool use_uv = (in_uv.z == 1.0 ? true : false);

	if (use_uv)
	{
		out_color = texture(texture_sampler, in_uv.xy);
	}

	else
	{
		out_color = in_color;
	}
}
