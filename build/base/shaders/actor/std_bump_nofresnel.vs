#version 330

layout(location=0) in vec3 iPosition;
layout(location=1) in vec2 iTexCoord;
layout(location=2) in vec3 iNormal;
layout(location=3) in vec4 iColors;
layout(location=4) in ivec4 iBoneIDs;
layout(location=5) in vec4 iWeights;
layout(location=6) in vec3 iTangent;

out vec2 TexCoord;
out vec3 Normal;
out vec3 WorldPos;
out vec4 Colors;
out vec3 Tangent;

const int lMaxBones = 100;

uniform bool gAnimated;
uniform mat4 gView;
uniform mat4 gModel;
uniform mat4 gNormalTransform;
uniform mat4 gBones[lMaxBones];

void main() {
	mat4 lBoneTransform = mat4(1.f);
	float animFactor = 1.f;
	if( gAnimated ) {
		lBoneTransform += gBones[iBoneIDs[0]] * iWeights[0];
		lBoneTransform += gBones[iBoneIDs[1]] * iWeights[1];
		lBoneTransform += gBones[iBoneIDs[2]] * iWeights[2];
		lBoneTransform += gBones[iBoneIDs[3]] * iWeights[3];
		animFactor = 16384.f;
	}
	
	vec4 lPos    = gModel * lBoneTransform * vec4(iPosition, 1.0) / animFactor;
	gl_Position  = gView * lPos;
	
	mat4 lNormalTransform = gNormalTransform * lBoneTransform;
	vec4 lNormal          = lNormalTransform * vec4(iNormal, 0.0);
	vec4 lTangent         = lNormalTransform * vec4(iTangent, 0.0);
	
	TexCoord     = iTexCoord;
	Normal       = normalize(lNormal.xyz);
	WorldPos     = lPos.xyz;
	Colors       = iColors;
	Tangent      = normalize(lTangent.xyz);
}