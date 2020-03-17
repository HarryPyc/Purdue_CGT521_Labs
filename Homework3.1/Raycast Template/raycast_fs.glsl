#version 430

layout(location = 1) uniform int pass;
layout(location = 3) uniform int mode = 0;
layout(location = 6) uniform float time;
layout(location = 7) uniform vec4 slider;
layout(location = 8) uniform int scene = 0;

layout(binding = 0) uniform sampler2D backfaces_tex;

layout(location = 0) out vec4 fragcolor;  
         
in vec3 vpos;  

//forward function declarations
vec4 raytracedcolor(vec3 rayStart, vec3 rayStop);
vec4 clear_color(vec3 rayDir);
vec4 lighting(vec3 pos, vec3 rayDir);
float distToShape(vec3 pos);
vec3 normal(vec3 pos);

const vec3 light_pos = vec3(5.0, 5.0, 5.0);

const vec4 La = vec4(0.75, 0.75, 0.75, 1.0);
const vec4 Ld = vec4(0.74, 0.74, 0.74, 1.0);
const vec4 Ls = vec4(1.0, 1.0, 0.74, 1.0);

const vec4 Ka = vec4(0.4, 0.4, 0.34, 1.0);
const vec4 Kd = vec4(1.0, 1.0, 0.73, 1.0);
const vec4 Ks = vec4(0.1, 0.1, 0.073, 1.0);


void main(void)
{   
	if(pass == 1)
	{
		fragcolor = vec4((vpos), 1.0); //write cube positions to texture
	}
	else if(pass == 2) 
	{
		if(mode == 0) // for debugging: show backface colors
		{
			fragcolor = texelFetch(backfaces_tex, ivec2(gl_FragCoord), 0);
			return;
		}
		else if(mode == 1) // for debugging: show frontface colors
		{
			fragcolor = vec4((vpos), 1.0);
			return;
		}
		else // raycast
		{
			vec3 rayStart = vpos.xyz;
			vec3 rayStop = texelFetch(backfaces_tex, ivec2(gl_FragCoord.xy), 0).xyz;
			fragcolor = raytracedcolor(rayStart, rayStop);
		}
	}
}


// trace rays until they intersect the surface
vec4 raytracedcolor(vec3 rayStart, vec3 rayStop)
{
	const int MaxSamples = 1000; //max number of steps along ray

	vec3 rayDir = normalize(rayStop-rayStart);	//ray direction unit vector
	float travel = distance(rayStop, rayStart);	
	float stepSize = travel/MaxSamples;	//initial raymarch step size
	vec3 pos = rayStart;				//position along the ray
	vec3 step = rayDir*stepSize;		//displacement vector along ray
	
	for (int i=0; i < MaxSamples && travel > 0.0; ++i, pos += step, travel -= stepSize)
	{
		float dist = distToShape(pos); //How far are we from the shape we are raycasting?

		//Distance tells us how far we can safely step along ray without intersecting surface
		stepSize = dist;
		step = rayDir*stepSize;
		
		//Check distance, and if we are close then perform lighting
		const float eps = 1e-4;
		if(dist <= eps)
		{
			return lighting(pos, rayDir);
		}	
	}
	//If the ray never intersects the scene then output clear color
	return clear_color(rayDir);
}

float shadow(in vec3 ro, in vec3 rd, float mint, float maxt);
float softshadow(in vec3 ro, in vec3 rd, float mint, float maxt, float w);
//Compute lighting on the raycast surface using Phong lighting model
vec4 lighting(vec3 pos, vec3 rayDir)
{
	const vec3 light = normalize(light_pos-pos); //light direction from surface
	vec3 n = normal(pos);

	vec4 La = clear_color(n);
	float sha = shadow(pos, light, 0.01, 3.0);
	float diff = max(0.0, dot(n, light)) * sha ;

	return La*Ka + Ld*Kd*diff;	
}

vec4 clear_color(vec3 rayDir)
{
	const vec4 color1 = vec4(mix(rayDir, vec3(1.0),0.8), 1.0);
	return color1;
}



//shape function declarations
float sdSphere( vec3 p, float s );
float sdBox( vec3 p, vec3 b );
float sdTorus(vec3 p, vec2 t);
float sdOctahedron(vec3 p, float s);
float sdCappedCylinder(vec3 p, float h, float r);
float sdPlane(vec3 p, float h);
// For more distance functions see
// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm

// Soft shadows
// http://www.iquilezles.org/www/articles/rmshadows/rmshadows.htm

// WebGL example and a simple ambient occlusion approximation
// https://www.shadertoy.com/view/Xds3zN


//distance to the shape we are drawing
float distToShape(vec3 pos)
{
	if(scene == 0)
	{
		const vec2 t = vec2(0.5f,0.1f*(sin(time)+1.5f));
		vec3 offset = 2.0*slider.xyz;
		float d0 = sdTorus(pos+offset, t);
		return d0;
	}

	else if(scene == 1)
	{
		float s = 0.5f*(slider.w + 1.f);
		float h = 0.5f;
		vec3 offset = 2.0*slider.xyz;
		float d1 = sdOctahedron(pos + offset, s);
		float d2 = sdPlane(pos, h);
		return min(d1,d2);
	}

	else if(scene == 2)
	{
		const float h = 0.5f, r = 0.1f, s = 0.5f;
		float d1 = sdOctahedron(pos, s);
		float d2 = sdCappedCylinder(pos, r, h);
		return max(d1,-d2);
	}
}

// shape function definitions      
float sdOctahedron(vec3 p, float s)
{
	p = abs(p);
	return (p.x + p.y + p.z - s)*0.57735027;
}
float sdTorus(vec3 p, vec2 t)
{
	vec2 q = vec2(length(p.xz) - t.x, p.y);
	return length(q) - t.y;
}  
float sdCappedCylinder(vec3 p, float h, float r)
{
	vec2 d = abs(vec2(length(p.xz), p.y)) - vec2(h, r);
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}
float sdSphere( vec3 p, float s )
{
	return length(p)-s;
}
float sdPlane(vec3 p, float h) {
	return p.z + h;
}
float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

//normal vector of the shape we are drawing.
//Estimated as the gradient of the signed distance function.
vec3 normal(vec3 pos)
{
	const float h = 0.001;
	const vec3 Xh = vec3(h, 0.0, 0.0);	
	const vec3 Yh = vec3(0.0, h, 0.0);	
	const vec3 Zh = vec3(0.0, 0.0, h);	

	return normalize(vec3(distToShape(pos+Xh)-distToShape(pos-Xh), distToShape(pos+Yh)-distToShape(pos-Yh), distToShape(pos+Zh)-distToShape(pos-Zh)));
}
float softshadow(in vec3 ro, in vec3 rd, float mint, float maxt, float w)
{
	float s = 1.0;
	for (float t = mint; t < maxt; )
	{
		float h = distToShape(ro + rd * t);
		s = min(s, 0.5 + 0.5*h / (w*t));
		if (s < 0.0) break;
		t += h;
	}
	s = max(s, 0.0);
	return s * s*(3.0 - 2.0*s); // smoothstep
}
float shadow(in vec3 ro, in vec3 rd, float mint, float maxt)
{
	for (float t = mint; t < maxt; )
	{
		float h = distToShape(ro + rd * t);
		if (h < 0.001)
			return 0.0;
		t += h;
	}
	return 1.0;
}
