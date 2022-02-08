#version 330 core

// vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
// instance attributes
layout (location = 2) in mat4 aModel;
layout (location = 6) in vec3 aColor;

out vec2 tex_coords;
out vec3 obj_color;

uniform mat4 view_proj;

void main()
{
    gl_Position = view_proj * aModel * vec4(aPos, 1.0);
    tex_coords = aTexCoords;
    obj_color = aColor;
}