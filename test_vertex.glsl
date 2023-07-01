#version 420 core

layout(location=0) in vec3 position;
layout(location=1) in mat4 model;

struct ps_input
{
	vec4 position;
};

out ps_input vs_output;

layout (std140, binding=0) uniform uni
{
	mat4  view_proj;
};

void main()
{
	vs_output.position = view_proj * vec4(position, 1);

	gl_Position = vs_output.position;
}
