#version 330 core

in vec3 fragColor;
in vec2 texCoords;

out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
	//FragColor = vec4(fragColor, 1);
	FragColor = texture(uTexture, texCoords) * vec4(fragColor, 1); 
}