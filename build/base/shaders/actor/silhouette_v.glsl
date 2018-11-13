#version 330

layout(location=0) in vec3 iPosition;
layout(location=4) in ivec4 iBoneIDs;
layout(location=5) in vec4 iWeights;

out vec3 Pos;

const int lMaxBones = 100;

uniform bool gAnimated;
uniform mat4 gBones[lMaxBones];
uniform mat4 gModel;
uniform mat4 gView;

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

	vec4 lPos   = gModel * lBoneTransform * vec4(iPosition, 1.0) / animFactor;
	gl_Position = gView * lPos;
	Pos         = lPos.xyz;
}