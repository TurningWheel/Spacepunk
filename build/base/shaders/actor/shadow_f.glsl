#version 330

in vec3 WorldPos;

uniform vec3 gLightPos;

out vec4 FragColor;

void main() {
	vec3 lLightToVertex = WorldPos - gLightPos;
	FragColor = vec4(length(lLightToVertex), 0.f, 0.f, 1.f);
}