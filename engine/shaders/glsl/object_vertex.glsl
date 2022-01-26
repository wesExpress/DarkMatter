#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoords;

out vec3 fragColor;
out vec2 texCoords;

uniform mat4 model;
uniform mat4 view_proj;
uniform vec3 offset;

void main()
{
    gl_Position = view_proj * model * vec4(aPos + offset, 1.0);
    fragColor = aColor;
    texCoords = aTexCoords;
}