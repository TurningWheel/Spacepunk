#version 400

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragColorBright;

uniform vec3 gCameraPos;

#define MAX_LIGHTS 12
uniform bool gActiveLight;
uniform vec3 gLightPos[MAX_LIGHTS];
uniform vec3 gLightColor[MAX_LIGHTS];
uniform float gLightIntensity[MAX_LIGHTS];
uniform float gLightRadius[MAX_LIGHTS];
uniform float gLightArc[MAX_LIGHTS];
uniform vec3 gLightScale[MAX_LIGHTS];
uniform vec3 gLightDirection[MAX_LIGHTS];
uniform int gLightShape[MAX_LIGHTS];
#ifndef NOSHADOWMAP
uniform mat4 gLightProj[MAX_LIGHTS];
uniform samplerCubeShadow gShadowmap[MAX_LIGHTS];
uniform usamplerCube gUIDmap[MAX_LIGHTS];
uniform bool gShadowmapEnabled[MAX_LIGHTS];
#endif
uniform int gNumLights;
uniform uint gUID;

#ifndef VERTEX_COLORS
#ifdef BUMPMAP
uniform sampler2D gTexture[2]; // 1st = diffuse, 2nd = normals
#else
uniform sampler2D gTexture;
#endif
#endif

// colormapped texture
uniform bool gCustomColorEnabled;
uniform vec4 gCustomColorR;
uniform vec4 gCustomColorG;
uniform vec4 gCustomColorB;
uniform vec4 gCustomColorA;

#ifndef NOSHADOWMAP
float ShadowFactor(int light)
{
	if (gShadowmapEnabled[light] == false) {
		return 1.f;
	}

	vec3 lLightDir = WorldPos - gLightPos[light];
	vec3 lLightDirNormal = normalize(lLightDir);
	vec3 lDiff = abs(lLightDir) - vec3(4.f);
	float lDist = min(-max(lDiff.x, max(lDiff.y, lDiff.z)), 0.f);
	vec4 lClip = gLightProj[light] * vec4(0.f, 0.f, lDist, 1.f);
	float lDepth = lClip.z / lClip.w;

	float lClampedDist = clamp(lDepth, 0.f, 1.f);
	vec4 lUVC = vec4(lLightDirNormal, lClampedDist);
	float lSample = 0.f;
	uvec4 lUID = uvec4(0);

	// glsl does not let you index a sampler
	switch (light) {
		case 0: lSample = texture(gShadowmap[0], lUVC); lUID = texture(gUIDmap[0], lUVC.xyz); break;
		case 1: lSample = texture(gShadowmap[1], lUVC); lUID = texture(gUIDmap[1], lUVC.xyz); break;
		case 2: lSample = texture(gShadowmap[2], lUVC); lUID = texture(gUIDmap[2], lUVC.xyz); break;
		case 3: lSample = texture(gShadowmap[3], lUVC); lUID = texture(gUIDmap[3], lUVC.xyz); break;
		case 4: lSample = texture(gShadowmap[4], lUVC); lUID = texture(gUIDmap[4], lUVC.xyz); break;
		case 5: lSample = texture(gShadowmap[5], lUVC); lUID = texture(gUIDmap[5], lUVC.xyz); break;
		case 6: lSample = texture(gShadowmap[6], lUVC); lUID = texture(gUIDmap[6], lUVC.xyz); break;
		case 7: lSample = texture(gShadowmap[7], lUVC); lUID = texture(gUIDmap[7], lUVC.xyz); break;
		case 8: lSample = texture(gShadowmap[8], lUVC); lUID = texture(gUIDmap[8], lUVC.xyz); break;
		case 9: lSample = texture(gShadowmap[9], lUVC); lUID = texture(gUIDmap[9], lUVC.xyz); break;
		case 10: lSample = texture(gShadowmap[10], lUVC); lUID = texture(gUIDmap[10], lUVC.xyz); break;
		case 11: lSample = texture(gShadowmap[11], lUVC); lUID = texture(gUIDmap[11], lUVC.xyz); break;
	}

	return (lUID.r == gUID) ? 1.f : lSample;
}
#endif

#ifndef VERTEX_COLORS
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
#ifndef VERTEX_COLORS
#ifdef BUMPMAP
		vec4 lTexColor = texture2D(gTexture[0], lTexCoord);
#else
		vec4 lTexColor = texture2D(gTexture, lTexCoord);
#endif
#else
		vec4 lTexColor = vec4(Colors.xyz, 1.f);
		if (!gActiveLight) {
			lTexColor = vec4(Colors.a, 0.f, 0.f, 1.f);
		}
#endif
		if( gActiveLight ) {
			FragColor  = gCustomColorR * lTexColor.r;
		} else {
			FragColor  = gCustomColorA * lTexColor.r;
		}
		FragColor += gCustomColorG * lTexColor.g;
		FragColor += gCustomColorB * lTexColor.b;
	} else {
#ifndef VERTEX_COLORS
#ifdef BUMPMAP
		FragColor = texture(gTexture[0], lTexCoord);
#else
		FragColor = texture(gTexture, lTexCoord);
#endif
#else
		if (gActiveLight) {
			FragColor = vec4(Colors.xyz, 1.f);
		} else {
			FragColor = vec4(Colors.a, 0.f, 0.f, 1.f);
		}
#endif
	}

	// light properties
#ifndef NOLIGHTING
	if( gActiveLight ) {
		vec4 lTotalLightColor = vec4(0.f);
		for( int c = 0; c < gNumLights; ++c ) {
			vec3 lLightDirection = normalize(gLightPos[c] - WorldPos);
			vec4 lLightColor = vec4(0.f);

			// stops stencil shadow z-fighting
			if( dot(Normal, lLightDirection) <= 0.0 ) {
				continue;
			}

#ifndef NOSHADOWMAP
			float lShadowFactor = ShadowFactor(c);
			if (lShadowFactor <= 0.f) {
				continue;
			}
#else
			float lShadowFactor = 1.f;
#endif
			lLightColor = vec4(gLightColor[c], 0.0);

			if( gLightShape[c] == 0 ) {
				// sphere
				vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
				float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
				float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				lLightColor.a = lLightFalloff * gLightIntensity[c];
			} else if( gLightShape[c] == 1 ) {
				// box
				vec3   lLightFragDiff  = (WorldPos - gLightPos[c]) * (WorldPos - gLightPos[c]);
				float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
				float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				lLightColor.a = lLightFalloff * gLightIntensity[c];
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
					float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.0) / lLightRadius;
					lLightColor.a = lLightFalloff * gLightIntensity[c] * lSpotFactor;
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
					float  lLightRadius    = gLightRadius[c] * gLightRadius[c];
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
					lLightColor.a = lLightFalloff * gLightIntensity[c] * lSpotFactor;
				} else {
					lLightColor.a = 0.0;
				}
			}

#ifndef NONORMALS
			lLightColor.a *= dot(lNormal, lLightDirection);
#endif
			lLightColor.a = lLightColor.a * lShadowFactor;
			lTotalLightColor += vec4(lLightColor.xyz * lLightColor.a, lLightColor.a);
		}

		FragColor *= lTotalLightColor;
	}
#endif
#ifdef FRESNEL
	vec3 cameraDir = normalize(gCameraPos - lWorldPos);
	float fresnelPower = pow(dot(lNormal, cameraDir), 2);
	FragColor = FragColor / fresnelPower;
#endif

	// check whether fragment output is higher than threshold, if so output to bloom buffer
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        FragColorBright = vec4(FragColor.rgb, 1.f);
    } else {
        FragColorBright = vec4(0.f, 0.f, 0.f, 1.f);
    }
}