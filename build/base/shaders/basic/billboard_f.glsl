#version 330

in vec2 TexCoord;

out vec4 FragColor;

uniform vec4 gColor;
uniform sampler2D gTexture;

void main() {
	FragColor = gColor * texture(gTexture, TexCoord);
}