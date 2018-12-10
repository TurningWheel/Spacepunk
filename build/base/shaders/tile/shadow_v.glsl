#version 330

layout(location=0) in vec3 iPosition;

uniform mat4 gView;

void main() {
	vec4 lPos    = vec4(iPosition, 1.0);
	gl_Position  = gView * lPos;
}