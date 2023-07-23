#version 460 core

layout(location=0) in vec3 position;
layout(location=1) in mat4 model;
layout(location=5) in vec4 color;

struct ps_input
{
	vec4 position;
	vec4 color;
};

layout(location=0) out ps_input vs_output;

#if 0
layout (std140, binding=0) uniform uni
{
	mat4  view_proj;
};
#endif

void main()
{
	//vs_output.position = view_proj * model * vec4(position, 1);
	vs_output.position = vec4(position, 1);
	vs_output.color = color;

	gl_Position = vs_output.position;
}
