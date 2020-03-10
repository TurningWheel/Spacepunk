#version 400

in vec2 TexCoord;

uniform sampler2D gTexture;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragColorBright;

void main() {
	FragColor = texture(gTexture, TexCoord) + vec4(1.f, 0.f, 1.f, 0.5f);
	FragColorBright = vec4(0.f);
}