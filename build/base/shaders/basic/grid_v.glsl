#version 330

layout(location=0) in vec3 iPosition;
layout(location=1) in vec4 iColor;

out vec4 Color;
out vec3 WorldPos;

uniform mat4 gView;
uniform vec3 gCameraPos;

void main() {
	const int gridSquare = 32 * 8;
	float distX = (int(gCameraPos.x) / gridSquare) * gridSquare - gridSquare * 16;
	float distZ = (int(gCameraPos.z) / gridSquare) * gridSquare - gridSquare * 16;
	vec3 Position = iPosition + vec3(distX, 0.f, distZ);
	gl_Position = gView * vec4(Position, 1.0);
	WorldPos = Position;
	Color = iColor;
}