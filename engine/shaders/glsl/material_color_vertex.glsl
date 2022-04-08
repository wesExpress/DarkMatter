#version 330 core

// vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
// instance attributes
layout (location = 3) in mat4 aModel;
layout (location = 7) in vec3 aDiffuse;
layout (location = 8) in vec3 aSpecular;
layout (location = 9) in float aShininess;

out vec3 normal;
out vec2 tex_coords;
out vec3 obj_color;
out vec3 frag_pos;

out vec3 diffuse_color;
out vec3 specular_color;
out float shininess;

uniform mat4 view_proj;

void main()
{
    gl_Position = view_proj * aModel * vec4(aPos, 1.0);
    
    normal = mat3(transpose(inverse(aModel))) * aNormal;
    tex_coords = aTexCoords;
    frag_pos = vec3(aModel * vec4(aPos, 1.0));

	diffuse_color = aDiffuse;
	specular_color = aSpecular;
	shininess = aShininess;
}