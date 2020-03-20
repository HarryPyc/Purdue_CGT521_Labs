#version 430

layout(location = 1) uniform int pass;
layout(location = 2) uniform int lightMode = 0;
layout(location = 3) uniform int mode = 0;
layout(location = 6) uniform float time;
layout(location = 7) uniform vec4 slider;
layout(location = 8) uniform int scene = 0;
layout(location = 9) uniform vec4 Ka;
layout(location = 10) uniform vec4 Kd;
layout(location = 11) uniform vec4 Ks;
layout(location = 12) uniform vec3 cam;
layout(location = 13) uniform float IOR = 2.5f;
layout(location = 14) uniform float m = 0.05f;

layout(binding = 0) uniform sampler2D backfaces_tex;

layout(location = 0) out vec4 fragcolor;  

const float PI = 3.1415926535897932384626433832795;
const float eps = 1e-6;
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
const vec4 Ls = vec4(1.0, 1.0, 1.0, 1.0);

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
//Cook-Torrance BRDF
float F(float IOR, vec3 n, vec3 v);
float D(float m, vec3 n, vec3 h);
float G(vec3 n, vec3 l, vec3 v, vec3 h);
//Compute lighting on the raycast surface using Phong lighting model
vec4 lighting(vec3 pos, vec3 rayDir)
{
	const vec3 light = normalize(light_pos-pos); //light direction from surface
	vec3 n = normal(pos);
	vec3 v = normalize(cam - pos);
	vec3 h = normalize(light + v);
	vec4 La = clear_color(n);
	float sha = softshadow(pos, light, 0.01, 3.0, 0.8);
	float diff = max(0.0, dot(n, light));
	float spec = 1 / (PI*dot(n, v)*dot(n, light) + eps);
	if (lightMode == 0)
		spec *= F(IOR, n, v)*D(m, n, h)*G(n, light, v, h);
	else if (lightMode == 1)
		spec *= F(IOR, n, v);
	else if (lightMode == 2)
		spec *= D(m, n, h);
	else if (lightMode == 3)
		spec *= G(n, light, v, h);
	return La*Ka + (Ld*Kd*diff + Ls*Ks*spec)*sha;	
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
		float d2 = sdPlane(pos, 0.5);
		return min(d0, d2);
	}

	else if(scene == 1)
	{
		float s = 0.5f*(slider.w + 1.f);
		vec3 offset = 2.0*slider.xyz;
		float d1 = sdOctahedron(pos + offset, s);
		return d1;
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
float softshadow(in vec3 ro, in vec3 rd, float mint, float maxt, float k)
{
	float res = 1.0;
	float ph = 1e20;
	for (float t = mint; t < maxt; )
	{
		float h = distToShape(ro + rd * t);
		if (h < 0.001)
			return 0.0;
		float y = h * h / (2.0*ph);
		float d = sqrt(h*h - y * y);
		res = min(res, k*d / max(0.0, t - y));
		ph = h;
		t += h;
	}
	return res;
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

float F(float IOR, vec3 n, vec3 v) {
	float F0 = (1 - IOR) / (1 + IOR);
	F0 = F0 * F0;
	float cosT = dot(n, v);
	return F0 + (1 - F0)*pow(1 - cosT, 5);
}
float D(float m, vec3 n, vec3 h) {
	float cosA2 = dot(n, h);
	cosA2 = cosA2 * cosA2;
	float tanA2 = (1 - cosA2) / cosA2;
	float m2 = m * m;
	return exp(-tanA2 / m2) / (4 * m2*cosA2*cosA2 + eps);
}
float G(vec3 n, vec3 l, vec3 v, vec3 h) {
	float temp = 2 * dot(n, h) / (dot(h, v)+eps);
	float Gm = dot(n, v)*temp;
	float Gs = dot(n, l)*temp;
	return min(1.f, min(Gm, Gs));
}