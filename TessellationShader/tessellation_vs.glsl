#version 430

in vec3 pos_attrib;

void main(void)
{
	 gl_Position = vec4(0.5*pos_attrib, 1.0);
}