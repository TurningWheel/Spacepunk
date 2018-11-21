#version 330

in Vertex {
	vec2 TexCoord;
	flat int Texture;
} vertex;

out vec4 FragColor;

uniform vec4 gColor;
uniform sampler2D gTexture[2];

void main() {
	FragColor = gColor * texture(gTexture[vertex.Texture], vertex.TexCoord);
}