#version 400

uniform sampler2D texture;
uniform sampler2D texture2;
uniform int pass;
uniform int blur;

out vec4 fragcolor;           
in vec2 tex_coord;
in vec3 l, p, n;
in float dist;

vec3 Ra = vec3(0.6, 0.6, 0.0), Rd = vec3(0.6, 0.6, 0.0);
float Kd = 3.0f, Ka = 0.5f;
vec3 tex_color;
vec4 f1, f2;//fbo texture 1 and 2
void main(void)
{   
	if(pass == 1 || pass == 2)
	{
		tex_color = Ra * Ka + Rd * Kd *max(dot(n, l), 0) / (dist * dist);
		fragcolor = vec4(tex_color, 1.0);
	}
	else if (pass == 3) {
		fragcolor = texture2D(texture, tex_coord);
	}
	else if(pass == 4)
	{
      //Lab assignment: Use texelFetch function and gl_FragCoord instead of using texture coordinates when reading from texture.
		f1 = texture2D(texture, tex_coord);
		f2 = texture2D(texture2, tex_coord);
		if (f1.w != 0)
			fragcolor = f1;
		else
			fragcolor = f2;
	}
	else
	{
		fragcolor = vec4(1.0, 0.0, 0.0, 1.0); //error
	}

}



















