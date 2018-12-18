#version 450

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

out vec4 FragColor;

uniform sampler2D gTexture;

// colormapped texture
uniform bool gCustomColorEnabled;
uniform vec4 gCustomColorR;
uniform vec4 gCustomColorG;
uniform vec4 gCustomColorB;
uniform vec4 gCustomColorA;

void main() {
	if( (mod(gl_FragCoord.x,4.f)<2.f) ^^ (mod(gl_FragCoord.y,4.f)<2.f) ) {
		FragColor = vec4( 0.f, 0.f, 0.f, 1.f );
	} else {
		vec2 lTexCoord = TexCoord;

		// do custom colors
		if( gCustomColorEnabled ) {
			vec4 TexColor = texture2D(gTexture, lTexCoord);
			FragColor  = gCustomColorA * TexColor.r;
			FragColor += gCustomColorG * TexColor.g;
			FragColor += gCustomColorB * TexColor.b;
		} else {
			FragColor = texture2D(gTexture, lTexCoord);
		}
	}
}