#version 430

layout(location = 0) out vec4 fragcolor;   
layout(location = 1) uniform float time;
 
float rand(vec2 co);
void main(void)
{  
	vec2 coord = gl_PointCoord.xy - vec2(0.5f);
	vec2 seed = vec2(float(gl_PrimitiveID), time);
	if (length(coord) >= 0.5f || length(coord) <= abs(0.25f*(1+sin(time+ float(gl_PrimitiveID))))) discard;
	fragcolor = vec4(rand(seed.xx), rand(seed.xy), rand(seed.yy), 1.0f);
	//fragcolor = vec4(1.0, 0.6, 0.0, 1.0);
}

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}



















