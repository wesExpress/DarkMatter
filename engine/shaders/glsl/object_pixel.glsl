#version 330 core

in vec2 tex_coords;
in vec3 obj_color;

out vec4 FragColor;

uniform sampler2D uTexture1;
uniform sampler2D uTexture2;

uniform vec3 global_light;
uniform float light_str;

void main()
{
	//FragColor = mix(texture(uTexture1, tex_coords), texture(uTexture2, tex_coords), 0.2);
	FragColor = vec4(light_str * global_light * obj_color, 1);
}