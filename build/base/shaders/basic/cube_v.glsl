#version 330

layout(location=0) in vec3 iPosition;
layout(location=1) in vec2 iTexCoord;
layout(location=2) in vec3 iNormal;

out vec2 TexCoord;
out vec3 Normal;

uniform mat4 gView;

void main() {
	vec4 lPos    = vec4(iPosition, 1.0);
	gl_Position  = gView * lPos;
	
	TexCoord    = iTexCoord;
	Normal      = iNormal;
}