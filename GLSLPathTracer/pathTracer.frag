#define PI 3.141592

uniform float iTime;
uniform vec2 iResolution;

#define randPI 3.14159265359

uint hash3(uint x, uint y, uint z) {
    x += x >> 11;
    x ^= x << 7;
    x += y;
    x ^= x << 3;
    x += z ^ (x >> 14);
    x ^= x << 6;
    x += x >> 15;
    x ^= x << 5;
    x += x >> 12;
    x ^= x << 9;
    return x;
}

//float rand(vec2 co){
//    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
//}

float randOld(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

//generate random uniform value between -1.0 and 1.0
float rand(vec2 pos, float time) {
    uint mantissaMask = 0x007FFFFFu;
    uint one = 0x3F800000u;
    uvec3 u = floatBitsToUint(vec3(pos, time));
    uint h = hash3(u.x, u.y, u.z);
    return uintBitsToFloat((h & mantissaMask) | one) - 1.0;
}

vec2 circleRand(vec3 rand){
//    rand += 1.;
//    rand /= 2.;
    float t = 2.*randPI*rand.x;
    float u = rand.y+rand.z;
    float r = 0.;
    if(u > 1.)  r = 2-u;
    else        r = u;
    return vec2(r*cos(t), r*sin(t));
}

//generates a random point on the surface of a sphere
vec3 sphereRandSurface(vec2 rand) {
    float ang1 = (rand.x*2. + 1.0) * randPI; // [-1..1) -> [0..2*PI)
    float u = rand.y*2.-1.; // [-1..1), cos and acos(2v-1) cancel each other out, so we arrive at [-1..1)
    float u2 = u * u;
    float sqrt1MinusU2 = sqrt(1.0 - u2);
    float x = sqrt1MinusU2 * cos(ang1);
    float y = sqrt1MinusU2 * sin(ang1);
    float z = u;
    return vec3(x, y, z);
}

//generates a point on the surface of the unit-hemisphere
vec3 hemiRand(vec2 rand, vec3 n) {
    vec3 v = sphereRandSurface(rand);
    return v * sign(dot(v, n));
}

float toRadians(float angle) {
	return (angle * PI) / 180.;
}

float toDegrees(float theta) {
	return (theta * 180.) / PI;
}

vec3 rotateX(vec3 v, float angle) {
	angle = toRadians(angle);

	float x = v.x;
	float y = v.y*cos(angle) - v.z*sin(angle);
	float z = v.y*sin(angle) + v.z*cos(angle);

	return vec3(x, y, z);
}

vec3 rotateY(vec3 v, float angle) {
	angle = toRadians(angle);

	float x = v.x*cos(angle) + v.z*sin(angle);
	float y = v.y;
	float z = -v.x*sin(angle) + v.z*cos(angle);

	return vec3(x, y, z);
}

vec3 rotateZ(vec3 v, float angle) {
	angle = toRadians(angle);

	float x = v.x*cos(angle) - v.y*sin(angle);
	float y = v.x*sin(angle) + v.y*cos(angle);
	float z = v.z;

	return vec3(x, y, z);
}

vec3 rotate(vec3 v, float x, float y, float z) {
	return rotateZ(rotateY(rotateX(v, x), y), z);
}

vec3 rotate(vec3 v, vec3 r) {
	return rotate(v, r.x, r.y, r.z);
}

uniform vec3 camPos;
uniform vec3 rot;
uniform float fovMult;

uniform float aperture;
uniform float focusDist;
uniform float focusInterval;

uniform int spp;
uniform int curSpp;
uniform int sppRate;

uniform int bounces;

int rSpp;

uniform sampler2D tex;

struct Material{
	vec3 albedo;
	float metallic;
	float roughness;
	float emit;
};

struct Sphere{
	vec3 pos;
	float radius;
	Material mat;
};

struct Plane{
	vec3 pos;
	vec3 normal;
	Material mat;
};

struct Ray{
	vec3 orig;
	vec3 dir;
};

struct Intersection{
	vec3 point;
	vec3 normal;
	float dist;
	Material mat;
};

const int maxSpheres = 10;
uniform int numSpheres;
uniform Sphere sp[maxSpheres];

const int maxPlanes = 10;
uniform int numPlanes;
uniform Plane pl[maxPlanes];

uniform float ambientIntensity; // = 2.;
uniform vec3 ambient; // = vec4(0.9*ambientIntensity, 1.0*ambientIntensity, 1.0*ambientIntensity, 1.0);
float absorption = 0.7;

vec2 uv;

Intersection miss;

Intersection intersects(Ray ray, Sphere sp){
	// Check for a Negative Square Root
    vec3 oc = sp.pos - ray.orig;
    float l = dot(ray.dir, oc);
    float det = pow(l, 2.0) - dot(oc, oc) + pow(sp.radius, 2.0);
    if (det < 0.0) return miss;

    // Find the Closer of Two Solutions
             float len = l - sqrt(det);
    if (len < 0.0) len = l + sqrt(det);
    if (len < 0.0) return miss;

	vec3 point = ray.orig+ray.dir*len;
    return Intersection(point, normalize(point-sp.pos), len, sp.mat); //(ray.orig + len*ray.dir - sp.pos) / sp.radius, len, sp.mat);
}

Intersection intersects(Ray ray, Plane p){
	vec3 norm = -p.normal;
	float denom = dot(norm, ray.dir);
	float t = 0.;
	if(denom > 0.0000001){
		vec3 p0l0 = p.pos - ray.orig;
		t = dot(p0l0, norm)/denom;
		vec3 point = ray.orig + ray.dir*t;
		//if(p.normal.y == 1.){
		//	//p.mat.albedo = (mod(int(point.x), 2) == 1 && mod(int(point.z), 2) == 0) || (mod(int(point.x), 2) == 0 && mod(int(point.z), 2) == 1) ? vec3(0.1) : vec3(1.);
		//	p.mat.albedo = int((floor(point.x*1.) + floor(point.z*1.)))%2 == 0 ? vec3(0.1) : vec3(0.9);
		//}
		return t >= 0. ? Intersection(point, -norm, t, p.mat) : miss;
	}
	
	return miss;
}

Intersection trace(Ray ray){
	Intersection inters = miss;
	float dist = 1e09;

	for(int i=0; i<numPlanes; i++){
		Intersection tInters = intersects(ray, pl[i]);
		if(tInters != miss){
			if(tInters.dist < dist){
				inters = tInters;
				dist = tInters.dist;
			}
		}
	}

	for(int i=0; i<numSpheres; i++){
		Intersection tInters = intersects(ray, sp[i]);
		if(tInters != miss){
			if(tInters.dist < dist){
				inters = tInters;
				dist = tInters.dist;
			}
		}
	}

	return inters;
}

float fresnel(vec3 norm, vec3 rDir, float metallic){
    return pow(1. - clamp(dot(norm, rDir), 0., 1.), 5.) * (1. - metallic) + metallic;
}

vec3 getRandHemiDeg(vec3 normal, vec2 coX, vec2 coY, vec2 coZ, float deg){
	return normalize(rotate(normal, vec3(rand(coX, iTime)*deg-deg/2., rand(coY, iTime)*deg-deg/2., rand(coZ, iTime)*deg-deg/2.)));
}

vec3 getRandHemi(vec3 normal, vec2 coX, vec2 coY, vec2 coZ){
	return getRandHemiDeg(normal, coX, coY, coZ, 180.);
}

struct CoordinateSystem{
	vec3 N;
	vec3 Nt;
	vec3 Nb;
};

CoordinateSystem createCoordinateSystem(vec3 N){
	CoordinateSystem cs;

	cs.N = N;

	if (abs(N.x) > abs(N.y))
		cs.Nt = vec3(N.z, 0., -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else
		cs.Nt = vec3(0., -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	cs.Nb = cross(N, cs.Nt);

	return cs;
}

vec3 uniformSampleHemisphere(float r1, float r2){
	float sinTheta = sqrt(1. - r1 * r1);
	float phi = 2. * PI * r2;
	float x = sinTheta * cos(phi);
	float z = sinTheta * sin(phi);
	return vec3(x, r1, z);
}

vec3 cosineSampleHemisphere(float u1, float u2){
	float r = sqrt(u1);
	float theta = 2. * PI * u2;

	float x = r * cos(theta);
	float y = r * sin(theta);

	return vec3(x, sqrt(max(0., 1. - u1)), y);
}

vec3 render(Ray ray){
	Intersection inters = trace(ray);

	vec3 color = vec3(0.);
	vec3 atten = vec3(1.);

	if(inters != miss){
		if(inters.mat.emit <= 0.){
			for(int i=0; i<3; i++){
				if(inters == miss){
					break;
				}

				vec3 difColor = vec3(0.);
				
				float fr = 0.;
				if(inters.mat.metallic > 0.){
					fr = fresnel(inters.normal, -ray.dir, inters.mat.metallic);
				}

				if(inters.mat.emit > 0.){
					color += inters.mat.albedo*inters.mat.emit*atten*(1.-fr);
					break;
				}

				vec3 indirAtten = vec3(1.);
				vec3 accumColor = inters.mat.albedo;

				Ray iRay;
				Intersection iInters = inters;
				Intersection prevIinters = inters;
				for(int gi=0; gi<bounces; gi++){
					prevIinters = iInters;

//					CoordinateSystem cs = createCoordinateSystem(iInters.normal);
//					float u1 = randOld(uv*vec2(13.343, -43.432)+iTime*87.234+float(rSpp)*-24.4+float(gi)*43.2);
//					float u2 = randOld(uv*vec2(24.343, -52.432)+iTime*4.96+float(rSpp)*2.145+float(gi)*-9.12);
//					vec3 sample = cosineSampleHemisphere(u1, u2);
//
//					vec3 rDir = vec3(
//							sample.x * cs.Nb.x + sample.y * iInters.normal.x + sample.z * cs.Nt.x,
//							sample.x * cs.Nb.y + sample.y * iInters.normal.y + sample.z * cs.Nt.y,
//							sample.x * cs.Nb.z + sample.y * iInters.normal.z + sample.z * cs.Nt.z);

					vec3 rDir = hemiRand(vec2(rand(uv, iTime+rSpp+gi), rand(-uv*2., iTime+rSpp+gi)), iInters.normal);

					iRay = Ray(iInters.point+iInters.normal*0.001, rDir); //getRandHemiDeg(iInters.normal, uv+iTime+float(rSpp)*1.3+float(gi)*4.6, uv*vec2(88.32, -22.234)+iTime*1.2+float(rSpp)*2.7+float(gi)*6.2, uv*vec2(-7.3,9.1)+iTime*3.5+float(rSpp)*9.4+float(gi)*1.1, 180.));
					iInters = trace(iRay);

					if(iInters != miss){
						float cos0 = max(0., dot(iRay.dir, prevIinters.normal));
						if(iInters.mat.emit > 0.){
							//float dist = length(prevIinters.point - iInters.point);
							vec3 col = (iInters.mat.albedo * iInters.mat.emit)  * cos0;
							difColor += accumColor * col*absorption*indirAtten/(cos0/(PI));
							break;
						}
						else{
							accumColor += iInters.mat.albedo* cos0 * indirAtten;
						}
					}
					else{
						difColor += inters.mat.albedo*ambient*ambientIntensity*absorption*indirAtten;
						break;
					}
					
					indirAtten *= 0.5;
				}

				color += difColor/PI*atten*(1.-fr);
				atten *= fr;

				if(inters.mat.metallic > 0.){
					vec3 refl = reflect(ray.dir, inters.normal);
					ray = Ray(inters.point+inters.normal*0.001, refl);

					inters = trace(ray);
				}
				else{
					break;
				}
			}
			
		}
		else{
			color += inters.mat.albedo*inters.mat.emit;
		}
	}
	else{
		color += ambient*ambientIntensity;
	}

	return color;
}

vec3 up = vec3(0.,1.,0.);

void main(void){
	if(curSpp < spp){
		uv = gl_FragCoord.xy/iResolution.xy*2.0-1.0;
		
		vec3 defPos = camPos;
		vec3 defDir = rotate(normalize(vec3(uv.x, uv.y*(iResolution.y/iResolution.x), fovMult)), rot);
		
		vec3 dofColor = vec3(0.);

		vec3 rRight = cross(defDir, up);
		vec3 rUp = cross(defDir, rRight);
		vec3 focusPoint = camPos+defDir*focusDist;
		
		vec3 fDir = normalize(focusPoint-camPos);

		float fStepUnit = focusInterval/float(spp);

		for(rSpp=curSpp; rSpp<curSpp+sppRate; rSpp++){
			
//			vec2 dofPosVar = vec2(randOld(uv+iTime+float(rSpp)*1.4)*2.-1., randOld(uv*vec2(1.63,-5.12)+iTime*2.1+float(rSpp)*3.6)*2.-1.)*aperture;
			vec2 dofPosVar = circleRand(vec3(randOld(uv+iTime), randOld(-uv*vec2(1.23,-3.12)+iTime), randOld(uv*vec2(5.13,-2.12)+iTime)))*aperture;
			vec3 newPos = camPos+rRight*dofPosVar.x+rUp*dofPosVar.y;

//			vec3 dofDir = normalize((focusPoint+fDir*fStepUnit*(float(rSpp)/2.)*(mod(rSpp, 2) == 0 ? 1. : -1.))-newPos);
			vec3 dofDir = normalize(focusPoint-newPos);

			Ray ray = Ray(newPos, dofDir);

			//dofColor += clamp(render(ray), 0., 1.);
			//dofColor = clamp(dofColor, 0., 1.);

			dofColor += render(ray);
		}
		
		//dofColor /= float(sppRate);

		vec3 color = dofColor;

		vec3 texel = vec3(0.);
		if(curSpp > 0){
			texel = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
		}

		gl_FragColor = vec4(texel.rgb + color.rgb, 1.);
	}
	else{
		gl_FragColor = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);
	}
}