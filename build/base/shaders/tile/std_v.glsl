#version 450

layout(location=0) in vec3 iPosition;
layout(location=1) in vec3 iNormal;
layout(location=2) in vec3 iTangent;
layout(location=3) in vec3 iDiffuseMap;
layout(location=4) in vec3 iNormalMap;
layout(location=5) in vec3 iEffectsMap;

out vec3 DiffuseMap;
out vec3 NormalMap;
out vec3 EffectsMap;
out vec3 Tangent;
out vec3 Normal;
out vec3 WorldPos;

uniform mat4 gView;

void main() {
	vec4 lPos    = vec4(iPosition, 1.0);
	gl_Position  = gView * lPos;
	
	WorldPos    = iPosition;
	Normal      = iNormal;
	Tangent     = iTangent;
	DiffuseMap  = iDiffuseMap;
	NormalMap   = iNormalMap;
	EffectsMap  = iEffectsMap;
}