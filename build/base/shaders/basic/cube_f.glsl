#version 330

in vec2 TexCoord;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D gTexture;
uniform vec4 gColor;

void main() {
	vec2 lTexCoord = TexCoord;
	vec3 lNormal   = Normal;

	FragColor = gColor * (1+Normal.x/4) * (1+Normal.y/2) * (1+Normal.z/8);
	FragColor.a = gColor.a;
}