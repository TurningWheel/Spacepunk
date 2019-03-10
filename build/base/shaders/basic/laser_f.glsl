#version 330

in Vertex {
	vec2 TexCoord;
	flat int Texture;
} vertex;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragColorBright;

uniform vec4 gColor;
uniform sampler2D gTexture[2];

void main() {
	FragColor = gColor * texture(gTexture[vertex.Texture], vertex.TexCoord);
	FragColorBright = FragColor;
}