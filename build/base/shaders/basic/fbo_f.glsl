#version 430

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D gTexture;

void main() {
	FragColor = vec4(texture(gTexture, TexCoord).rgb, 1.f);
}