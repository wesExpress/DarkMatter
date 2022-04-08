#version 330 core

in vec3 normal;
in vec2 tex_coords;
in vec3 frag_pos;

out vec4 FragColor;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform float shininess;

uniform vec3 light_pos;
uniform vec3 light_ambient;
uniform vec3 light_diffuse;
uniform vec3 light_specular;

uniform vec3 view_pos;

void main()
{
	vec3 norm_normal = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);
	vec3 view_dir = normalize(view_pos - frag_pos);
	vec3 reflect_dir = reflect(-light_dir, norm_normal);

	vec3 ambient = light_ambient * texture(diffuse_map, tex_coords).rgb;

	float diff = max(dot(norm_normal, light_dir), 0.0);
	diff = 0.01;
	vec3 diffuse = light_diffuse * diff * texture(diffuse_map, tex_coords).rgb;

	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	vec3 specular = light_specular * spec * texture(specular_map, tex_coords).rgb;

	FragColor = vec4((ambient + diffuse + specular), 1);
}