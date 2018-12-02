#version 450

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

out vec4 FragColor;

uniform bool gAnimated;
uniform bool gActiveLight;
uniform vec3 gCameraPos;

#define MAX_LIGHTS 32
uniform mat4 gLightProj;
uniform vec3 gLightPos[MAX_LIGHTS];
uniform vec4 gLightColor[MAX_LIGHTS];
uniform float gLightIntensity[MAX_LIGHTS];
uniform float gLightRadius[MAX_LIGHTS];
uniform vec3 gLightScale[MAX_LIGHTS];
uniform vec3 gLightDirection[MAX_LIGHTS];
uniform int gLightShape[MAX_LIGHTS];
#ifndef NOSHADOWMAP
uniform samplerCube gShadowmap[MAX_LIGHTS];
uniform bool gShadowmapEnabled[MAX_LIGHTS];
#endif
uniform int gNumLights;

#ifdef BUMPMAP
uniform sampler2D gTexture[2]; // 1st = diffuse, 2nd = normals
#else
uniform sampler2D gTexture;
#endif

// colormapped texture
uniform bool gCustomColorEnabled;
uniform vec4 gCustomColorR;
uniform vec4 gCustomColorG;
uniform vec4 gCustomColorB;
uniform vec4 gCustomColorA;

#ifndef NOSHADOWMAP
bool IsInShadow(int shadow, vec3 lightDirection)
{
	if (gShadowmapEnabled[shadow] == false) {
		return false;
	}

	float lDist = length(lightDirection);
	float lSample = 0.f;
	
	// glsl does not let you index a sampler
	switch (shadow) {
		case 0: lSample = texture(gShadowmap[0], -normalize(lightDirection)).r; break;
		case 1: lSample = texture(gShadowmap[1], -normalize(lightDirection)).r; break;
		case 2: lSample = texture(gShadowmap[2], -normalize(lightDirection)).r; break;
		case 3: lSample = texture(gShadowmap[3], -normalize(lightDirection)).r; break;
		case 4: lSample = texture(gShadowmap[4], -normalize(lightDirection)).r; break;
		case 5: lSample = texture(gShadowmap[5], -normalize(lightDirection)).r; break;
		case 6: lSample = texture(gShadowmap[6], -normalize(lightDirection)).r; break;
		case 7: lSample = texture(gShadowmap[7], -normalize(lightDirection)).r; break;
		case 8: lSample = texture(gShadowmap[8], -normalize(lightDirection)).r; break;
		case 9: lSample = texture(gShadowmap[9], -normalize(lightDirection)).r; break;
		case 10: lSample = texture(gShadowmap[10], -normalize(lightDirection)).r; break;
		case 11: lSample = texture(gShadowmap[11], -normalize(lightDirection)).r; break;
		case 12: lSample = texture(gShadowmap[12], -normalize(lightDirection)).r; break;
		case 13: lSample = texture(gShadowmap[13], -normalize(lightDirection)).r; break;
		case 14: lSample = texture(gShadowmap[14], -normalize(lightDirection)).r; break;
		case 15: lSample = texture(gShadowmap[15], -normalize(lightDirection)).r; break;
		case 16: lSample = texture(gShadowmap[16], -normalize(lightDirection)).r; break;
		case 17: lSample = texture(gShadowmap[17], -normalize(lightDirection)).r; break;
		case 18: lSample = texture(gShadowmap[18], -normalize(lightDirection)).r; break;
		case 19: lSample = texture(gShadowmap[19], -normalize(lightDirection)).r; break;
		case 20: lSample = texture(gShadowmap[20], -normalize(lightDirection)).r; break;
		case 21: lSample = texture(gShadowmap[21], -normalize(lightDirection)).r; break;
		case 22: lSample = texture(gShadowmap[22], -normalize(lightDirection)).r; break;
		case 23: lSample = texture(gShadowmap[23], -normalize(lightDirection)).r; break;
		case 24: lSample = texture(gShadowmap[24], -normalize(lightDirection)).r; break;
		case 25: lSample = texture(gShadowmap[25], -normalize(lightDirection)).r; break;
		case 26: lSample = texture(gShadowmap[26], -normalize(lightDirection)).r; break;
		case 27: lSample = texture(gShadowmap[27], -normalize(lightDirection)).r; break;
		case 28: lSample = texture(gShadowmap[28], -normalize(lightDirection)).r; break;
		case 29: lSample = texture(gShadowmap[29], -normalize(lightDirection)).r; break;
		case 30: lSample = texture(gShadowmap[30], -normalize(lightDirection)).r; break;
		case 31: lSample = texture(gShadowmap[31], -normalize(lightDirection)).r; break;
	}

	if (lDist < lSample) {
		return false;
	} else {
		return true;
	}
}
#endif

#ifdef BUMPMAP
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
#endif

void main() {
	vec2   lTexCoord  = TexCoord;
#ifdef BUMPMAP
	vec3   lNormal    = CalcBumpedNormal();
#else
	vec3   lNormal    = Normal;
#endif
	vec3   lWorldPos  = WorldPos;

	// do custom colors
	if( gCustomColorEnabled ) {
#ifdef BUMPMAP
		vec4 lTexColor = texture2D(gTexture[0], lTexCoord);
#else
		vec4 lTexColor = texture2D(gTexture, lTexCoord);
#endif
		if( gActiveLight ) {
			FragColor  = gCustomColorR * lTexColor.r;
		} else {
			FragColor  = gCustomColorA * lTexColor.r;
		}
		FragColor += gCustomColorG * lTexColor.g;
		FragColor += gCustomColorB * lTexColor.b;
	} else {
#ifdef BUMPMAP
		FragColor = texture(gTexture[0], lTexCoord);
#else
		FragColor = texture(gTexture, lTexCoord);
#endif
	}

	// light properties
	if( gActiveLight ) {
		vec4 lTotalLightColor = vec4(0.f);
		for( int c = 0; c < gNumLights; ++c ) {
			vec3 lLightDirection = normalize(gLightPos[c] - WorldPos);
			vec4 lLightColor = vec4(0.f);

			// stops stencil shadow z-fighting
			if( dot(Normal, lLightDirection) <= 0.0 ) {
				continue;
			}

			// this texel is shadowed
#ifndef NOSHADOWMAP
			if( IsInShadow(c, gLightPos[c] - WorldPos) == true ) {
				continue;
			}
#endif

			lLightColor = gLightColor[c];

			if( gLightShape[c] == 0 ) {
				// sphere
				vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
				float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
				float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.f), 1.f);
			} else if( gLightShape[c] == 1 ) {
				// box
				vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
				float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
				float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.f), 1.f);
			} else if( gLightShape[c] == 2 ) {
				// capsule
			} else if( gLightShape[c] == 3 ) {
				// cylinder
			} else if( gLightShape[c] == 4 ) {
				// cone
				float  lSpotFactor = dot(-lLightDirection, gLightDirection[c]);
				if( lSpotFactor > 0.0 ) {
					lSpotFactor = pow(lSpotFactor, 10);
					vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
					float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
					float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.0) / lLightRadius;
					lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.0), 1.0) * lSpotFactor;
				} else {
					lLightColor.a = 0.0;
				}
			} else if( gLightShape[c] == 5 ) {
				// pyramid
				float  lSpotFactor = dot(-lLightDirection, gLightDirection[c]);
				if( lSpotFactor > 0.0 ) {
					lSpotFactor = pow(lSpotFactor, 10);
					vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
					float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
					float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
					lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.0), 1.0) * lSpotFactor;
				} else {
					lLightColor.a = 0.0;
				}
			}

#ifndef NONORMALS
			lLightColor.a *= dot(lNormal, lLightDirection);
#endif
			lTotalLightColor += vec4(lLightColor.xyz * lLightColor.a, lLightColor.a);
		}

		FragColor *= lTotalLightColor;

#ifdef FRESNEL
		vec3 cameraDir = normalize(gCameraPos - lWorldPos);
		FragColor /= dot(lNormal, cameraDir);
#endif
	}
}