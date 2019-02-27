#version 450

layout(location = 0) in vec4 in_color;

#if0
layout(binding = 0) uniform sampler2D texture_sampler;
layout(location = 1) in vec3 in_uv;
#endif

layout(location = 0) out vec4 out_color;

void main () {
	#if 0
	bool use_uv = (in_uv.z == 1.0 ? true : false);

	if (use_uv)
	{
		out_color = texture(texture_sampler, uv.xy);
	}

	else
	{
		out_color = in_color;
	}
	#else
	out_color = in_color;
	#endif
}
