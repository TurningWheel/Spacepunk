#version 330

layout(location=0) in vec3 iPosition;
layout(location=1) in vec2 iTexCoord;
layout(location=2) in vec3 iNormal;
layout(location=3) in vec3 iTangent;

out vec3 WorldPos;
out vec2 TexCoord;
out vec3 Normal;
out vec3 Tangent;

uniform mat4 gView;

void main() {
	vec4 lPos   = vec4(iPosition, 1.0);
	gl_Position = gView * lPos;
	
	WorldPos = iPosition;
	TexCoord = iTexCoord;
	Normal   = iNormal;
	Tangent  = iTangent;
}