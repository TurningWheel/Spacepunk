#version 430

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2DMS gTexture;
uniform ivec2 gResolution;

void main() {
	FragColor  = texelFetch(gTexture, ivec2(TexCoord * gResolution), 0) * 0.25f;
	FragColor += texelFetch(gTexture, ivec2(TexCoord * gResolution), 1) * 0.25f;
	FragColor += texelFetch(gTexture, ivec2(TexCoord * gResolution), 2) * 0.25f;
	FragColor += texelFetch(gTexture, ivec2(TexCoord * gResolution), 3) * 0.25f;
}