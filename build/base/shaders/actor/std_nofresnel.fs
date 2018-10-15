#version 330

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

out vec4 FragColor;

uniform bool gAnimated;
uniform int gFullbright;
uniform vec3 gCameraPos;
uniform vec3 gLightPos;
uniform vec4 gLightColor;
uniform float gLightIntensity;
uniform float gLightRadius;
uniform sampler2D gTexture;

// colormapped texture
uniform bool gCustomColorEnabled;
uniform vec4 gCustomColorR;
uniform vec4 gCustomColorG;
uniform vec4 gCustomColorB;
uniform vec4 gCustomColorA;

void main() {
	vec2   lTexCoord  = TexCoord;
	vec3   lNormal    = Normal;
	vec3   lWorldPos  = WorldPos;
	vec3   lLightPos  = gLightPos;

	vec3   cameraDir  = normalize(gCameraPos - lWorldPos);
	vec3   lightDir   = normalize(gLightPos - lWorldPos);
	float  radius     = gLightRadius * gLightRadius;
	vec3   diff       = (lWorldPos - lLightPos) * (lWorldPos - lLightPos);
	float  dist       = (diff.x + diff.y + diff.z);

	float  falloff    = max(radius - dist, 0.f) / radius;

	// do custom colors
	if( gCustomColorEnabled ) {
		vec4 TexColor = texture(gTexture, lTexCoord);
		if( gFullbright==0 ) {
			FragColor  = gCustomColorR * TexColor.r;
		} else {
			FragColor  = gCustomColorA * TexColor.r;
		}
		FragColor += gCustomColorG * TexColor.g;
		FragColor += gCustomColorB * TexColor.b;
	} else {
		FragColor = texture(gTexture, lTexCoord);
	}

	// light properties
	if( gFullbright==0 ) {
		FragColor *= gLightColor * gLightIntensity;
		FragColor.a = dot(lNormal, lightDir) * falloff;

		// fresnel disabled
		//FragColor /= dot(lNormal, cameraDir);
	}
}