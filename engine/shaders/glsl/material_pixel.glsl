#version 330 core

in vec3 normal;
in vec2 tex_coords;
in vec3 frag_pos;

out vec4 FragColor;

uniform sampler2D diffuse_color;
uniform sampler2D specular_color;
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

	vec3 ambient = light_ambient * vec3(texture(diffuse_color, tex_coords));

	float diff = max(dot(norm_normal, light_dir), 0.0);
	vec3 diffuse = light_diffuse * diff * vec3(texture(diffuse_color, tex_coords));

	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	vec3 specular = light_specular * spec * vec3(texture(specular_color, tex_coords));

	FragColor = vec4((ambient + diffuse + specular), 1);
}