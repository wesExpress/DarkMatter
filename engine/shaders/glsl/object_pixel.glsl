#version 330 core

in vec2 texCoords;

out vec4 FragColor;

uniform sampler2D uTexture1;
uniform sampler2D uTexture2;

void main()
{
	FragColor = mix(texture(uTexture1, texCoords), texture(uTexture2, texCoords), 0.2);
}