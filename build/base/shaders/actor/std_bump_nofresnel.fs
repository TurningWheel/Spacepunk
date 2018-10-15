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
uniform sampler2D gTexture[2]; // 1st = diffuse, 2nd = normals

// colormapped texture
uniform bool gCustomColorEnabled;
uniform vec4 gCustomColorR;
uniform vec4 gCustomColorG;
uniform vec4 gCustomColorB;
uniform vec4 gCustomColorA;

vec3 CalcBumpedNormal() {
    vec3 lNormal = normalize(Normal);
    vec3 lTangent = normalize(Tangent);
    lTangent = normalize(lTangent - dot(lTangent, lNormal) * lNormal);
    vec3 lBitangent = cross(lTangent, lNormal);

    vec3 lBumpMapNormal = texture(gTexture[1], TexCoord).xyz;
    lBumpMapNormal = 2.0 * lBumpMapNormal - vec3(1.0, 1.0, 1.0);

    mat3 lTBN = mat3(lTangent, lBitangent, lNormal);
    vec3 lNewNormal = lTBN * lBumpMapNormal;
    lNewNormal = normalize(lNewNormal);
    return lNewNormal;
}

void main() {
	vec2   lTexCoord  = TexCoord;
	vec3   lNormal    = CalcBumpedNormal();
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
		vec4 TexColor = texture(gTexture[0], lTexCoord);
		if( gFullbright==0 ) {
			FragColor  = gCustomColorR * TexColor.r;
		} else {
			FragColor  = gCustomColorA * TexColor.r;
		}
		FragColor += gCustomColorG * TexColor.g;
		FragColor += gCustomColorB * TexColor.b;
	} else {
		FragColor = texture(gTexture[0], lTexCoord);
	}

	// light properties
	if( gFullbright==0 ) {
		FragColor *= gLightColor * gLightIntensity;
		FragColor.a = dot(lNormal, lightDir) * falloff;
		//FragColor /= dot(lNormal, cameraDir);
	}
}