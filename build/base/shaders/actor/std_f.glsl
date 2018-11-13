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
uniform vec3 gLightPos;
uniform vec4 gLightColor;
uniform float gLightIntensity;
uniform float gLightRadius;
uniform vec3 gLightScale;
uniform vec3 gLightDirection;
uniform int gLightShape;
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
	vec3   lLightPos  = gLightPos;

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
		vec3 lLightDirection = normalize(gLightPos - WorldPos);

		// stops stencil shadow z-fighting
		if( dot(Normal, lLightDirection) <= 0.0 ) {
			discard;
		}

		FragColor *= gLightColor;

		if( gLightShape == 0 ) {
			// sphere
			vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
			float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
			float  lLightRadius    = gLightRadius * gLightRadius;
			float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
			FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.f), 1.f);
		} else if( gLightShape == 1 ) {
			// box
			vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
			float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
			float  lLightRadius    = gLightRadius * gLightRadius;
			float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
			FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.f), 1.f);
		} else if( gLightShape == 2 ) {
			// capsule
		} else if( gLightShape == 3 ) {
			// cylinder
		} else if( gLightShape == 4 ) {
			// cone
			float  lSpotFactor = dot(-lLightDirection, gLightDirection);
			if( lSpotFactor > 0.0 ) {
				lSpotFactor = pow(lSpotFactor, 10);
				vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
				float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
				float  lLightRadius    = gLightRadius * gLightRadius;
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.0) / lLightRadius;
				FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.0), 1.0) * lSpotFactor;
			} else {
				FragColor.a = 0.0;
			}
		} else if( gLightShape == 5 ) {
			// pyramid
			float  lSpotFactor = dot(-lLightDirection, gLightDirection);
			if( lSpotFactor > 0.0 ) {
				lSpotFactor = pow(lSpotFactor, 10);
				vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
				float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
				float  lLightRadius    = gLightRadius * gLightRadius;
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.0), 1.0) * lSpotFactor;
			} else {
				FragColor.a = 0.0;
			}
		}

#ifndef NONORMALS
		FragColor.a *= dot(lNormal, lLightDirection);
#endif
#ifdef FRESNEL
		vec3 cameraDir = normalize(gCameraPos - lWorldPos);
		FragColor /= dot(lNormal, cameraDir);
#endif
	}
}