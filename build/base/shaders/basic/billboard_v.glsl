#version 400

layout(location=0) in vec3 iPosition;

uniform mat4 gModel;

void main() {
	gl_Position = gModel * vec4(iPosition, 1.0);
}