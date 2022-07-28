#version 330 core

// vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
// instance attributes
layout (location = 3) in mat4 aModel;

out vec3 normal;
out vec2 tex_coords;
out vec3 frag_pos;

layout (std140) uniform scene_uniform
{
	mat4 view_proj;
	vec4 light_ambient;
	vec4 light_diffuse;
	vec4 light_specular;
	vec4 light_pos;
	vec4 view_pos;
};

struct inst_data
{
	uint is_light;
	uint has_texture;
	float shininess;
	float padding;
};

layout (std140) uniform inst_uniform
{
	inst_data inst_data_array[1024];
};

void main()
{
	
}