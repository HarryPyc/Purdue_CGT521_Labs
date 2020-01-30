#version 430            
uniform mat4 PVM;
uniform mat4 PV;
layout(location = 0)uniform vec3 lightPos;
layout(location = 0)in vec3 pos_attrib;
layout(location = 1)uniform mat4 M;
layout(location = 1)in vec3 normal_attrib;
layout(location = 2)in vec2 tex_coord_attrib;

layout(location = 3)in vec4 M1;
layout(location = 4)in vec4 M2;
layout(location = 5)in vec4 M3;
layout(location = 6)in vec4 M4;
layout(location = 7)in vec3 color;

out vec3 Color;
out vec2 tex_coord; 

out vec3 dist;
out vec4 p;
out vec3 n;
out vec3 l;

mat4 T;
void main(void)
{
	T = mat4(M1, M2, M3, M4);
	gl_Position = PV* M * T * vec4(pos_attrib, 1.0);
   tex_coord = tex_coord_attrib;
   Color = color;

   p =   M *T* vec4(pos_attrib, 1.0);
   dist = p.xyz - lightPos;
   l = normalize(dist);
   n = -vec3(normalize(M * T * vec4(normal_attrib, 0.0)));
}