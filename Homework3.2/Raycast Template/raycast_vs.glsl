#version 430            

layout(location = 0) uniform mat4 PV;
layout(location = 1) uniform int pass; //which render pass is this?
layout(location = 4) uniform mat4 M;
layout(location = 5) uniform mat4 T; //view transformation

out vec3 vpos; 

const vec4 cube[8] = vec4[]( vec4(-1.0, -1.0, -1.0, 1.0),
							 vec4(-1.0, +1.0, -1.0, 1.0),
							 vec4(+1.0, +1.0, -1.0, 1.0),
							 vec4(+1.0, -1.0, -1.0, 1.0),
							 vec4(-1.0, -1.0, +1.0, 1.0),
							 vec4(-1.0, +1.0, +1.0, 1.0),
							 vec4(+1.0, +1.0, +1.0, 1.0),
							 vec4(+1.0, -1.0, +1.0, 1.0));

const int index[14] = int[](1, 0, 2, 3, 7, 0, 4, 1, 5, 2, 6, 7, 5, 4);

/* Draw the cube vertices as indexed strips with this function
void draw_attribless_cube()
{
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}
*/

void main(void)
{
	int ix = index[gl_VertexID];
	gl_Position = PV*M*cube[ix];
	vpos = vec3(T*M*cube[ix]);
}