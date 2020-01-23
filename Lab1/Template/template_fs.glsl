#version 430

layout(location = 4) uniform float time;
layout(location = 1) uniform sampler2D diffuse_color;

out vec4 fragcolor;           
in vec2 tex_coord;
      
void main(void)
{   
	fragcolor = texture(diffuse_color, tex_coord);
    fragcolor = vec4(0.5*sin(time)+0.5,0,0,1);
}




















