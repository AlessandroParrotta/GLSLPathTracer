
attribute vec4 coords;
varying out vec2 texCoord;

void main(void){
	//gl_Position = gl_Vertex;
	//gl_TexCoord[0] = gl_MultiTexCoord0;
	//texCoord = gl_TexCoord[0].st;
	
	gl_Position = vec4(coords.x, coords.y, 0., 1.);

	texCoord = coords.zw;
}