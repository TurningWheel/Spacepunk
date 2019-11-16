#version 330

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform mat4 gViewProj;
uniform mat4 gModelView;
uniform float gSize;

out vec2 TexCoord;

void main()
{
	mat4 view  = inverse(gModelView);
	vec4 right = view[0];
	vec4 up    = view[1];

	gl_Position = gViewProj * (gl_in[0].gl_Position - (right - up) * gSize);
	TexCoord = vec2(0.0, 0.0);
	EmitVertex();

	gl_Position = gViewProj * (gl_in[0].gl_Position - (right + up) * gSize);
	TexCoord = vec2(0.0, 1.0);
	EmitVertex();

	gl_Position = gViewProj * (gl_in[0].gl_Position + (right + up) * gSize);
	TexCoord = vec2(1.0, 0.0);
	EmitVertex();

	gl_Position = gViewProj * (gl_in[0].gl_Position + (right - up) * gSize);
	TexCoord = vec2(1.0, 1.0);
	EmitVertex();

	EndPrimitive();
}