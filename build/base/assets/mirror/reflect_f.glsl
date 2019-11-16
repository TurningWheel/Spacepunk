#version 330

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

out vec4 FragColor;

uniform vec3 gCameraPos;
uniform samplerCube gCubemap;

void main() {
	vec3 lNormal     = normalize(Normal);
	vec3 lWorldPos   = WorldPos;
	vec3 lCameraDir  = normalize(gCameraPos - lWorldPos);
	vec3 lReflection = reflect(normalize(lCameraDir), lNormal) * -1;
	FragColor = texture(gCubemap, lReflection);
}