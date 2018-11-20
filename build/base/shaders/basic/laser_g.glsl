#version 330

layout(lines) in;
layout(triangle_strip, max_vertices=4) out;

uniform mat4 gViewProj;
uniform mat4 gModelView;
uniform vec3 gCameraPos;
uniform float gWidth;

out vec2 TexCoord;

vec3 findNormal(vec3 a, vec3 b, vec3 c) {
	vec3 u = b - a;
	vec3 v = c - a;
	return vec3(
		u.y * v.z - u.z * v.y,
		u.z * v.x - u.x * v.z,
		u.x * v.y - u.y * v.x
	);
}

void main()
{
	vec4 p0 = gl_in[0].gl_Position;
	vec4 p1 = gl_in[1].gl_Position;
	vec4 normal = vec4(normalize(findNormal(p0.xyz, gCameraPos, p1.xyz)), 0.f);

	gl_Position = gViewProj * (p0 - normal * gWidth);
	TexCoord = vec2(0.0, 0.0);
	EmitVertex();

	gl_Position = gViewProj * (p1 - normal * gWidth);
	TexCoord = vec2(1.0, 0.0);
	EmitVertex();

	gl_Position = gViewProj * (p0 + normal * gWidth);
	TexCoord = vec2(0.0, 1.0);
	EmitVertex();

	gl_Position = gViewProj * (p1 + normal * gWidth);
	TexCoord = vec2(1.0, 1.0);
	EmitVertex();

	EndPrimitive();
}