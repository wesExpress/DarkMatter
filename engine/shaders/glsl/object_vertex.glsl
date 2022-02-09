#version 330 core

// vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
// instance attributes
layout (location = 3) in mat4 aModel;
layout (location = 7) in vec3 aColor;

out vec3 normal;
out vec2 tex_coords;
out vec3 obj_color;
out vec3 frag_pos;

uniform mat4 view_proj;

void main()
{
    gl_Position = view_proj * aModel * vec4(aPos, 1.0);
    
    normal = aNormal;
    tex_coords = aTexCoords;
    obj_color = aColor;
    frag_pos = vec3(aModel * vec4(aPos, 1.0));
}