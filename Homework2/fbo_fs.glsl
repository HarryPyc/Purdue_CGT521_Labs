#version 400

uniform sampler2D texture;
uniform int pass;
uniform int blur;
uniform bool edgeDetection;

          
in vec2 tex_coord;
flat in int instanceID;
layout(location = 0)out vec4 fragcolor;
layout(location = 1)out vec4 data;
void main(void)
{   
	if(pass == 1)
	{
		fragcolor = texture2D(texture, tex_coord);
		
		data = vec4(instanceID / 255.f);
	}
	else if(pass == 2)
	{
      //Lab assignment: Use texelFetch function and gl_FragCoord instead of using texture coordinates when reading from texture.
		if (edgeDetection) {
			vec4 left = texelFetch(texture, ivec2(gl_FragCoord.xy) - ivec2(1, 0), 0);
			vec4 right = texelFetch(texture, ivec2(gl_FragCoord.xy) + ivec2(1, 0), 0);
			vec4 above = texelFetch(texture, ivec2(gl_FragCoord.xy) + ivec2(0, 1), 0);
			vec4 below = texelFetch(texture, ivec2(gl_FragCoord.xy) - ivec2(0, 1), 0);
			fragcolor = pow(left - right, vec4(2)) + pow(above - below, vec4(2));
		}
		else
			fragcolor = texture2D(texture, tex_coord);
	}
	else
	{
		fragcolor = vec4(1.0, 0.0, 1.0, 1.0); //error
	}

}




















