#version 400

layout(location=0) in vec3 iPosition;
layout(location=1) in uint iIndex;

uniform mat4 gModel;
uniform mat4 gViewProj;
uniform vec3 gDiff;

void main() {
	vec4 lPos;

	if (iIndex == 1) {
		lPos = vec4(iPosition, 1.0);
	} else {
		lPos = vec4(iPosition * gDiff, 1.0);
	}

	gl_Position = gViewProj * gModel * lPos;
}