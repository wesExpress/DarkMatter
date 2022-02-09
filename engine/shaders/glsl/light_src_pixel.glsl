#version 330 core

in vec2 tex_coords;
in vec3 obj_color;

out vec4 frag_color;

uniform vec3 global_light;
uniform float light_str;

void main()
{
	frag_color = vec4(global_light * obj_color, 1);
}