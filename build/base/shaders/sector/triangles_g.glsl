#version 330

layout(triangles) in;
layout(line_strip, max_vertices=6) out;

in vec3 Pos[];

void EmitLine(int startIndex, int endIndex) {
	gl_Position = gl_in[startIndex].gl_Position;
	EmitVertex();

	gl_Position = gl_in[endIndex].gl_Position;
	EmitVertex();

	EndPrimitive();
}

void main()
{
	EmitLine(0, 1);
	EmitLine(1, 2);
	EmitLine(2, 0);
}