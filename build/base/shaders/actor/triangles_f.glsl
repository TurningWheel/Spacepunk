#version 330

out vec4 FragColor;

uniform vec4 gHighlightColor;

void main() {
	FragColor = gHighlightColor;
}