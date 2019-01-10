#version 400

in vec3 WorldPos;
in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;

out vec4 FragColor;

uniform bool gActiveLight;
uniform vec3 gCameraPos;
uniform vec3 gLightPos;
uniform vec4 gLightColor;
uniform float gLightIntensity;
uniform float gLightRadius;
uniform vec3 gLightScale;
uniform int gLightShape;
uniform sampler2D gTexture[3];
uniform samplerCube gCubemap;

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
	if( gActiveLight ) {
		vec3  lLightDirection  = normalize(gLightPos - WorldPos);
		vec3  lNormal          = CalcBumpedNormal();
		float lDiffuseFactor   = dot(lNormal, lLightDirection);

		vec4 lDiffuseColor  = vec4(0.0);
		vec4 lSpecularColor = vec4(0.0);

		// calculate diffuse color
		if( lDiffuseFactor > 0.0 ) {
			lDiffuseColor = gLightColor * max(gLightIntensity, 1.f) * lDiffuseFactor;

			// calculate specular color
			vec3  lFragToEye       = normalize(WorldPos - gCameraPos);
			vec3  lLightReflect    = normalize(reflect(lLightDirection, lNormal));
			float lSpecularFactor  = dot(lFragToEye, lLightReflect);

			if( lSpecularFactor > 0.0 ) {
				vec4  lEffectFrag         = texture(gTexture[2], TexCoord);
				float lSpecularIntensity  = lEffectFrag.r * 255.0;
				float lSpecularPower      = pow(lSpecularFactor, max(lEffectFrag.g * 255.0, 1.f));

				lSpecularColor = gLightColor * lSpecularIntensity * lSpecularPower;
			}

			// apply fragment
			FragColor = texture(gTexture[0], TexCoord) * (lDiffuseColor + lSpecularColor);

			// apply falloff
			vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
			float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
			float  lLightRadius    = gLightRadius * gLightRadius;
			float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
			FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.f), 1.f);
		} else {
			FragColor = vec4(0.0);		
		}
	} else {
		float lGlowFactor = texture(gTexture[2], TexCoord).b;
		FragColor = texture(gTexture[0], TexCoord) * lGlowFactor;
	}
}