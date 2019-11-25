#version 330

in vec4 Color;
in vec3 WorldPos;

out vec4 FragColor;

uniform vec3 gCameraPos;

void main() {
	vec3 diff = gCameraPos - WorldPos;
	float sum = abs(diff.x) + abs(diff.y) + abs(diff.z);
	float dist = max(sum / 256.f, 1.f);
	//float dist = 1.f;
	FragColor = Color / dist;
}