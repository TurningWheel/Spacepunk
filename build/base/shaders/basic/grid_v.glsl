#version 330

layout(location=0) in vec3 iPosition;
layout(location=1) in vec4 iColor;

out vec4 Color;

uniform mat4 gView;

void main() {
	gl_Position = gView * vec4(iPosition, 1.0);
	Color = iColor;
}