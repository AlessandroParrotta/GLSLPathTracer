uniform int curSpp;
uniform sampler2D tex;

in vec2 texCoord;

uniform float iTime;
uniform vec2 iResolution;
uniform vec2 fboRes;

void main(void){
//	vec3 c = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb/float(curSpp);
	vec3 c = texelFetch(tex, ivec2(texCoord.x*fboRes.x, texCoord.y*fboRes.y), 0).rgb/float(curSpp);

	//float gamma = 2.2*abs(sin(iTime)*10.+0.000001);
	//vec3 corrected = vec3(pow(c.x, 1.0/gamma), pow(c.y, 1.0/gamma), pow(c.z, 1.0/gamma));
//	gl_FragColor = vec4(texCoord.xy, 0., 1.);
	gl_FragColor = vec4(c, 1.);
}