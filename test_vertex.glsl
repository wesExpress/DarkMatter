#version 330 core

layout(location=0) in vec3 position;

struct ps_input
{
	vec4 position;
};

out ps_input vs_output;

void main()
{
	vs_output.position = vec4(position, 1);

	gl_Position = vs_output.position;
}
