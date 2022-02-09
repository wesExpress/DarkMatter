#version 330 core

in vec3 normal;
in vec2 tex_coords;
in vec3 obj_color;
in vec3 frag_pos;

out vec4 FragColor;

uniform sampler2D uTexture1;
uniform sampler2D uTexture2;

uniform vec3 global_light;
uniform float ambient;
uniform vec3 light_pos;

void main()
{
	vec3 norm_normal = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);

	float diffuse = max(dot(norm_normal, light_dir), 0.0);

	//FragColor = mix(texture(uTexture1, tex_coords), texture(uTexture2, tex_coords), 0.2);
	FragColor = vec4((ambient + diffuse) * global_light * obj_color, 1);
}