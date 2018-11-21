#version 330

layout(lines) in;
layout(triangle_strip, max_vertices=12) out;

uniform mat4 gViewProj;
uniform mat4 gModelView;
uniform vec3 gCameraPos;
uniform float gWidth;

out Vertex {
	vec2 TexCoord;
	flat int Texture;
} vertex;

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
	vec4 p2 = p1 + normalize(p1 - p0) * gWidth; // end cap
	vec4 p3 = p0 + normalize(p0 - p1) * gWidth; // start cap
	vec4 normal = vec4(normalize(findNormal(p0.xyz, gCameraPos, p1.xyz)), 0.f);

	// body
	gl_Position = gViewProj * (p0 - normal * gWidth);
	vertex.TexCoord = vec2(0.0, 0.0);
	vertex.Texture = 0;
	EmitVertex();

	gl_Position = gViewProj * (p1 - normal * gWidth);
	vertex.TexCoord = vec2(1.0, 0.0);
	vertex.Texture = 0;
	EmitVertex();

	gl_Position = gViewProj * (p0 + normal * gWidth);
	vertex.TexCoord = vec2(0.0, 1.0);
	vertex.Texture = 0;
	EmitVertex();

	gl_Position = gViewProj * (p1 + normal * gWidth);
	vertex.TexCoord = vec2(1.0, 1.0);
	vertex.Texture = 0;
	EmitVertex();

	EndPrimitive();

	// end cap
	gl_Position = gViewProj * (p1 - normal * gWidth);
	vertex.TexCoord = vec2(0.0, 0.0);
	vertex.Texture = 1;
	EmitVertex();

	gl_Position = gViewProj * (p2 - normal * gWidth);
	vertex.TexCoord = vec2(1.0, 0.0);
	vertex.Texture = 1;
	EmitVertex();

	gl_Position = gViewProj * (p1 + normal * gWidth);
	vertex.TexCoord = vec2(0.0, 1.0);
	vertex.Texture = 1;
	EmitVertex();

	gl_Position = gViewProj * (p2 + normal * gWidth);
	vertex.TexCoord = vec2(1.0, 1.0);
	vertex.Texture = 1;
	EmitVertex();

	EndPrimitive();

	// start cap
	gl_Position = gViewProj * (p3 - normal * gWidth);
	vertex.TexCoord = vec2(1.0, 0.0);
	vertex.Texture = 1;
	EmitVertex();

	gl_Position = gViewProj * (p0 - normal * gWidth);
	vertex.TexCoord = vec2(0.0, 0.0);
	vertex.Texture = 1;
	EmitVertex();

	gl_Position = gViewProj * (p3 + normal * gWidth);
	vertex.TexCoord = vec2(1.0, 1.0);
	vertex.Texture = 1;
	EmitVertex();

	gl_Position = gViewProj * (p0 + normal * gWidth);
	vertex.TexCoord = vec2(0.0, 1.0);
	vertex.Texture = 1;
	EmitVertex();

	EndPrimitive();
}