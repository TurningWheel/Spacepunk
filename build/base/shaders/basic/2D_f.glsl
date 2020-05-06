#version 330

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D gTexture;
uniform vec4 gColor;

void main() {
	FragColor = gColor * texture(gTexture, TexCoord);
	FragColor = clamp(FragColor, vec4(0.f), vec4(1.f));
}