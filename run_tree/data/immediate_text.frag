#version 450

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

// TODO(soimn): expose width and offset

void main ()
{
	vec4 value = texture(texture_sampler, in_uv);
	float alpha = 1.0 - smoothstep(0.5, 0.6, value.r);

	out_color = vec4(in_color.xyz, alpha);
}
