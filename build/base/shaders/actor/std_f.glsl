#version 330

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

out vec4 FragColor;

uniform bool gAnimated;
uniform bool gActiveLight;
uniform vec3 gCameraPos;

const int MAX_LIGHTS = 50;
uniform vec3 gLightPos[MAX_LIGHTS];
uniform vec4 gLightColor[MAX_LIGHTS];
uniform float gLightIntensity[MAX_LIGHTS];
uniform float gLightRadius[MAX_LIGHTS];
uniform vec3 gLightScale[MAX_LIGHTS];
uniform vec3 gLightDirection[MAX_LIGHTS];
uniform int gLightShape[MAX_LIGHTS];
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