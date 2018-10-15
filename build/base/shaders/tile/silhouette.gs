#version 330

layout(triangles_adjacency) in;
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
	vec3 e1 = Pos[2] - Pos[0];
	vec3 e2 = Pos[4] - Pos[0];
	vec3 e3 = Pos[1] - Pos[0];
	vec3 e4 = Pos[3] - Pos[2];
	vec3 e5 = Pos[4] - Pos[2];
	vec3 e6 = Pos[5] - Pos[0];

	vec3 Normal     = cross(e1,e2);
	vec3 lCameraPos = gCameraPos;
	vec3 LightDir   = lCameraPos - Pos[0];

	if( dot(Normal, LightDir) > 0 ) {
		Normal = cross(e3,e1);

		if (dot(Normal, LightDir) <= 0) {
			EmitLine(0, 2);
		}

		Normal = cross(e4,e5);
		LightDir = lCameraPos - Pos[2];

		if (dot(Normal, LightDir) <=0) {
			EmitLine(2, 4);
		}

		Normal = cross(e2,e6);
		LightDir = lCameraPos - Pos[4];

		if (dot(Normal, LightDir) <= 0) {
			EmitLine(4, 0);
		}
	}
}