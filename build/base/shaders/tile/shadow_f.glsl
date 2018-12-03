#version 330

in vec3 WorldPos;

uniform vec3 gLightPos;

out vec4 FragColor;

void main() {
	vec3 lLightToVertex = WorldPos - gLightPos;
	float lDist = lLightToVertex.x*lLightToVertex.x + lLightToVertex.y*lLightToVertex.y + lLightToVertex.z*lLightToVertex.z;
	FragColor = vec4(lDist, 0.f, 0.f, 1.f);
}