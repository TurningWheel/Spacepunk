#version 330

layout(location=0) in vec2 iPosition;
layout(location=1) in vec2 iTexCoord;

out vec2 TexCoord;

uniform mat4 gView;

void main() {
	vec4 lPos    = vec4(iPosition, 0.0, 1.0);
	gl_Position  = gView * lPos;
	
	TexCoord    = iTexCoord;
}