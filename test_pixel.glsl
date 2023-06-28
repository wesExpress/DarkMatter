#version 330 core

struct ps_input
{
	vec4 position;
};

out vec4 frag_color;

void main()
{
	frag_color = vec4(1,1,1,1);
}
