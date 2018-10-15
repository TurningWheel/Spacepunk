#version 330

layout(triangles) in;
layout(line_strip, max_vertices=6) out;

in vec3 Pos[];

uniform vec3 gCameraPos;

void EmitLine(int startIndex, int endIndex) {
	gl_Position = gl_in[startIndex].gl_Position;
	EmitVertex();

	gl_Position = gl_in[endIndex].gl_Position;
	EmitVertex();

	EndPrimitive();
}

void main()
{
	vec3 e1 = Pos[1] - Pos[0];
	vec3 e2 = Pos[2] - Pos[0];
	vec3 e3 = Pos[2] - Pos[1];

	vec3 Normal     = cross(e1,e2);
	vec3 lCameraPos = gCameraPos;
	vec3 LightDir   = lCameraPos - Pos[0];

	if( dot(Normal, LightDir) > 0 ) {
		EmitLine(0, 1);
		EmitLine(1, 2);
		EmitLine(2, 0);
	}
}