#version 430

uniform sampler2D diffuse_color;

out vec4 fragcolor;           
in vec2 tex_coord;
in vec3 Color;

in vec3 dist;
in vec4 p;
in vec3 n;
in vec3 l;

vec3 light;
vec3 ambient;
vec3 diffuse;

vec3 Ka = vec3(1.0);//ambient color
vec3 Kd = vec3(1.0);//diffuse color
float La = 0.2f;//ambient intensity
float Ld = 3.0f;//diffuse intensity

float Rd;
float attenuation;
void main(void)
{   
	vec4 tex_color = texture(diffuse_color, tex_coord) * vec4(Color, 1);

	//Phong lighting model
	Rd = max(dot(n, l), 0); 
	attenuation = 1 / pow(length(dist), 2);

	ambient = vec3(tex_color) * La; //ambient color
	diffuse = vec3(tex_color) * Rd * Ld;//diffuse color

	light = ambient + attenuation * (diffuse );

	fragcolor = vec4(light, 1.0);
}




















