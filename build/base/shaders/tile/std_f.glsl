#version 450

in vec3 DiffuseMap;
in vec3 NormalMap;
in vec3 EffectsMap;
in vec3 Normal;
in vec3 WorldPos;
in vec3 Tangent;

out vec4 FragColor;

#define CHUNK_SIZE 2
#define TILE_SIZE 128

uniform bool gActiveLight;
uniform vec3 gCameraPos;

#define MAX_LIGHTS 32
uniform vec3 gLightPos[MAX_LIGHTS];
uniform vec3 gLightColor[MAX_LIGHTS];
uniform float gLightIntensity[MAX_LIGHTS];
uniform float gLightRadius[MAX_LIGHTS];
uniform float gLightArc[MAX_LIGHTS];
uniform vec3 gLightScale[MAX_LIGHTS];
uniform vec3 gLightDirection[MAX_LIGHTS];
uniform int gLightShape[MAX_LIGHTS];
uniform mat4 gLightProj[MAX_LIGHTS];
uniform samplerCubeShadow gShadowmap[MAX_LIGHTS];
uniform bool gShadowmapEnabled[MAX_LIGHTS];
uniform int gNumLights;

uniform sampler2DArray gDiffuseMap;
uniform sampler2DArray gNormalMap;
uniform sampler2DArray gEffectsMap;
uniform samplerCube gCubeMap;
uniform mat3 gTileColors[CHUNK_SIZE * CHUNK_SIZE];

float ShadowFactor(int light)
{
	if (gShadowmapEnabled[light] == false) {
		return 1.f;
	}

	vec3 lLightDir = WorldPos - gLightPos[light];
	vec3 lLightDirNormal = -normalize(lLightDir);
	vec3 lDiff = abs(lLightDir) - vec3(3.f);
	float lDist = -max(lDiff.x, max(lDiff.y, lDiff.z));
	vec4 lClip = gLightProj[light] * vec4(0.0, 0.0, lDist, 1.0);
	float lDepth = (lClip.z / lClip.w) * 0.5f + 0.5f;

	float lClampedDist = clamp(lDepth, 0.f, 1.f);
	vec4 lUVC = vec4(-lLightDirNormal, lClampedDist);
	float lSample = 0.f;

	// glsl does not let you index a sampler
	switch (light) {
		case 0: lSample = texture(gShadowmap[0], lUVC); break;
		case 1: lSample = texture(gShadowmap[1], lUVC); break;
		case 2: lSample = texture(gShadowmap[2], lUVC); break;
		case 3: lSample = texture(gShadowmap[3], lUVC); break;
		case 4: lSample = texture(gShadowmap[4], lUVC); break;
		case 5: lSample = texture(gShadowmap[5], lUVC); break;
		case 6: lSample = texture(gShadowmap[6], lUVC); break;
		case 7: lSample = texture(gShadowmap[7], lUVC); break;
		case 8: lSample = texture(gShadowmap[8], lUVC); break;
		case 9: lSample = texture(gShadowmap[9], lUVC); break;
		case 10: lSample = texture(gShadowmap[10], lUVC); break;
		case 11: lSample = texture(gShadowmap[11], lUVC); break;
		case 12: lSample = texture(gShadowmap[12], lUVC); break;
		case 13: lSample = texture(gShadowmap[13], lUVC); break;
		case 14: lSample = texture(gShadowmap[14], lUVC); break;
		case 15: lSample = texture(gShadowmap[15], lUVC); break;
		case 16: lSample = texture(gShadowmap[16], lUVC); break;
		case 17: lSample = texture(gShadowmap[17], lUVC); break;
		case 18: lSample = texture(gShadowmap[18], lUVC); break;
		case 19: lSample = texture(gShadowmap[19], lUVC); break;
		case 20: lSample = texture(gShadowmap[20], lUVC); break;
		case 21: lSample = texture(gShadowmap[21], lUVC); break;
		case 22: lSample = texture(gShadowmap[22], lUVC); break;
		case 23: lSample = texture(gShadowmap[23], lUVC); break;
		case 24: lSample = texture(gShadowmap[24], lUVC); break;
		case 25: lSample = texture(gShadowmap[25], lUVC); break;
		case 26: lSample = texture(gShadowmap[26], lUVC); break;
		case 27: lSample = texture(gShadowmap[27], lUVC); break;
		case 28: lSample = texture(gShadowmap[28], lUVC); break;
		case 29: lSample = texture(gShadowmap[29], lUVC); break;
		case 30: lSample = texture(gShadowmap[30], lUVC); break;
		case 31: lSample = texture(gShadowmap[31], lUVC); break;
	}

	return lSample;
}

vec3 CalcBumpedNormal() {
	vec3 lNormal = normalize(Normal);
	vec3 lTangent = normalize(Tangent);
	lTangent = normalize(lTangent - dot(lTangent, lNormal) * lNormal);
	vec3 lBitangent = cross(lTangent, lNormal);

	vec3 lBumpMapNormal = texture(gNormalMap, NormalMap).xyz;
	lBumpMapNormal = 2.0 * lBumpMapNormal - vec3(1.0, 1.0, 1.0);

	mat3 lTBN = mat3(lTangent, lBitangent, lNormal);
	vec3 lNewNormal = lTBN * lBumpMapNormal;
	lNewNormal = normalize(lNewNormal);
	return lNewNormal;
}

vec4 MapDiffuseTexture(vec3 lNormal, float lReflectPower) {
	vec4 lTexColor = texture(gDiffuseMap, DiffuseMap);

	float size  = CHUNK_SIZE * TILE_SIZE;
	int index   = int(mod(floor(WorldPos.z + Normal.z), size) / TILE_SIZE);
	index	  += int(mod(floor(WorldPos.x + Normal.x), size) / TILE_SIZE) * CHUNK_SIZE;

	vec4 lFinalColor;
	lFinalColor  = vec4(gTileColors[index][0], 0.0) * lTexColor.r;
	lFinalColor += vec4(gTileColors[index][1], 0.0) * lTexColor.g;
	lFinalColor += vec4(gTileColors[index][2], 0.0) * lTexColor.b;
	lFinalColor += vec4(0.0, 0.0, 0.0, 1.0) * lTexColor.a;
	lFinalColor  = lFinalColor * (1.0 - lReflectPower);

	vec3 cameraDir	= normalize(gCameraPos - WorldPos);
	vec3 lReflection  = reflect(normalize(cameraDir), lNormal) * -1;
	lFinalColor	  += texture(gCubeMap, lReflection) * lReflectPower;

	return lFinalColor;
}

vec4 MapDiffuseTextureOLD(vec3 lNormal, float lReflectPower) {
	vec4 lTexColor   = texture(gDiffuseMap, DiffuseMap) * (1.0 - lReflectPower);

	vec3 cameraDir	 = normalize(gCameraPos - WorldPos);
	vec3 lReflection   = reflect(normalize(cameraDir), lNormal) * -1;
	vec4 lReflectColor = texture(gCubeMap, lReflection) * lReflectPower;

	return lTexColor + lReflectColor;
}

void main() {
	vec4  lEffectFrag   = texture(gEffectsMap, EffectsMap);
	float lReflectPower = lEffectFrag.a;
	vec3  lNormal	   = CalcBumpedNormal();

	if( !gActiveLight ) {
		float lGlowFactor = lEffectFrag.b;
		FragColor = MapDiffuseTexture(lNormal, lReflectPower) * lGlowFactor;
	} else {
		FragColor = MapDiffuseTexture(lNormal, lReflectPower);

		vec4 lTotalLightColor = vec4(0.f);
		for( int c = 0; c < gNumLights; ++c ) {
			vec3  lLightDirection = normalize(gLightPos[c] - WorldPos);

			// stops stencil shadow z-fighting
			if( dot(Normal, lLightDirection) <= 0.0 ) {
				continue;
			}

			float lShadowFactor   = ShadowFactor(c);
			float lDiffuseFactor  = dot(lNormal, lLightDirection);

			vec4 lDiffuseColor  = vec4(0.0);
			vec4 lSpecularColor = vec4(0.0);

			// calculate diffuse color
			if( lDiffuseFactor <= 0.f || lShadowFactor <= 0.f) {
				continue;
			} else {
				lDiffuseColor = vec4(gLightColor[c], 0.0) * max(gLightIntensity[c], 1.f) * lDiffuseFactor;

				// calculate specular color
				vec3  lFragToEye	   = normalize(WorldPos - gCameraPos);
				vec3  lLightReflect	= normalize(reflect(lLightDirection, lNormal));
				float lSpecularFactor  = dot(lFragToEye, lLightReflect);

				if( lSpecularFactor > 0.0 ) {
					float lSpecularIntensity  = lEffectFrag.r * 255.0;
					float lSpecularPower	  = pow(lSpecularFactor, max(lEffectFrag.g * 255.0, 1.f));

					lSpecularColor = vec4(gLightColor[c], 0.0) * lSpecularIntensity * lSpecularPower;
				}

				// apply lighting
				vec4 lLightColor = (lDiffuseColor + lSpecularColor);

				// apply falloff
				if( gLightShape[c] == 0 ) {
					// sphere
					vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
					float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
					float  lLightRadius	= gLightRadius[c] * gLightRadius[c];
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
					lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.f), 1.f);
				} else if( gLightShape[c] == 1 ) {
					// box
					vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
					float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
					float  lLightRadius	= gLightRadius[c] * gLightRadius[c];
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
					lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.f), 1.f);
				} else if( gLightShape[c] == 2 ) {
					// capsule
				} else if( gLightShape[c] == 3 ) {
					// cylinder
				} else if( gLightShape[c] == 4 ) {
					// cone
					float lCos = cos(gLightArc[c]);
					float lDot = dot(-lLightDirection, gLightDirection[c]);
					float lSpotFactor = max(lDot - lCos, 0.f) / (1.f - lCos);
					if( lSpotFactor > 0.0 ) {
						vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
						float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
						float  lLightRadius	= gLightRadius[c] * gLightRadius[c];
						float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.0) / lLightRadius;
						lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.0), 1.0) * lSpotFactor;
					} else {
						lLightColor.a = 0.0;
					}
				} else if( gLightShape[c] == 5 ) {
					// pyramid
					float lCos = cos(gLightArc[c]);
					float lDot = dot(-lLightDirection, gLightDirection[c]);
					float lSpotFactor = max(lDot - lCos, 0.f) / (1.f - lCos);
					if( lSpotFactor > 0.0 ) {
						vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
						float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
						float  lLightRadius	= gLightRadius[c] * gLightRadius[c];
						float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
						lLightColor.a = lLightFalloff * min( max(gLightIntensity[c], 0.0), 1.0) * lSpotFactor;
					} else {
						lLightColor.a = 0.0;
					}
				}
				lLightColor.a = clamp(lLightColor.a, 0.f, 1.f) * lShadowFactor;
				lTotalLightColor += vec4(lLightColor.xyz * lLightColor.a, lLightColor.a);
			}
		}
		FragColor *= lTotalLightColor;
	}
}