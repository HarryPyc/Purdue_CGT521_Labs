#version 400 
uniform mat4 P;           
uniform mat4 VM;
uniform mat4 Shadow;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;

out vec4 shadow_coord;

void main(void)
{
	 gl_Position = P*VM*vec4(pos_attrib, 1.0);
	 shadow_coord = Shadow*VM*vec4(pos_attrib, 1.0);
}