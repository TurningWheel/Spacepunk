#version 330

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D gTexture;
uniform vec4 gColor;

void main() {
	FragColor = gColor * texture(gTexture, TexCoord);
}