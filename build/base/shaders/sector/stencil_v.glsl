#version 330

layout(location=0) in vec3 iPosition;

out vec3 Pos;

void main() {
	Pos = iPosition;
}