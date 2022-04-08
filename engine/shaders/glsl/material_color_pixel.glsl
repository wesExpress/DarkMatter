#version 330 core

in vec3 normal;
in vec2 tex_coords;
in vec3 frag_pos;

in vec3 diffuse_color;
in vec3 specular_color;
in float shininess;

out vec4 FragColor;

uniform vec3 light_pos;
uniform vec3 light_ambient;
uniform vec3 light_diffuse;
uniform vec3 light_specular;
uniform float light_strength;

uniform vec3 view_pos;

void main()
{
	vec3 norm_normal = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);
	vec3 view_dir = normalize(view_pos - frag_pos);
	vec3 reflect_dir = reflect(-light_dir, norm_normal);

	vec3 ambient = light_ambient * diffuse_color;

	float diff = max(dot(norm_normal, light_dir), 0.0);
	vec3 diffuse = diff * diffuse_color * light_diffuse * light_strength;

	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	vec3 specular = spec * specular_color * light_specular;

	FragColor = vec4((ambient + diffuse + specular), 1);
}