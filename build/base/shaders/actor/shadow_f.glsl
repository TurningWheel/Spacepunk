#version 400

layout (location = 0) out uint FragColor;

uniform uint gUID;

void main() {
	FragColor = gUID;
}