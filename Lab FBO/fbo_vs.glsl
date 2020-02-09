#version 400            
uniform mat4 PVM;
uniform int pass;
uniform vec3 light;
uniform mat4 M;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;


out vec2 tex_coord;  
out vec3 l,p,n;
out float dist;

void main(void)
{
	if(pass == 1 || pass == 2)
	{
		gl_Position = PVM*vec4(pos_attrib, 1.0);
		tex_coord = tex_coord_attrib;
		p = vec3(M * vec4(pos_attrib, 1.0));
		n = normalize(vec3(M*vec4(normal_attrib, 1.0)));
		l = light - p;
		dist = length(l);
		l = normalize(l);
	}
	else if (pass == 3) {
		gl_Position = PVM * vec4(pos_attrib, 1.0);
		tex_coord = 0.5*pos_attrib.xy + vec2(0.5);
	}
	else
	{
		gl_Position = vec4(pos_attrib, 1.0);
      tex_coord = 0.5*pos_attrib.xy + vec2(0.5);
	}
}