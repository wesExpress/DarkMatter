#version 330 core

in vec3 normal;
in vec2 tex_coords;
in vec3 frag_pos;

out vec4 FragColor;

layout (std140) uniform scene_uniform
{
	mat4 view_proj;
	vec4 light_ambient;
	vec4 light_diffuse;
	vec4 light_specular;
	vec4 light_pos;
	vec4 view_pos;
};

void main()
{
	FragColor = vec4(1,1,1,1);
}