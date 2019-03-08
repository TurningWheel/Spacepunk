#version 430

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2DMS gTexture;
uniform ivec2 gResolution;

void main() {
#ifndef HDR
	vec3 Color = vec3(0.f);
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 0).rgb * 0.25f;
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 1).rgb * 0.25f;
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 2).rgb * 0.25f;
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 3).rgb * 0.25f;
	FragColor = vec4(Color, 1.f);
#else
	vec3 Color = vec3(0.f);
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 0).rgb * 0.25f;
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 1).rgb * 0.25f;
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 2).rgb * 0.25f;
	Color += texelFetch(gTexture, ivec2(TexCoord * gResolution), 3).rgb * 0.25f;

	// reinhard tone mapping
	//vec3 Mapped = Color / (Color + vec3(1.f));

	// exposure tone mapping
	const float exposure = 1.f;
	vec3 Mapped = vec3(1.f) - exp(-Color * exposure);

	// gamma correction
	const float gamma = 1.f;
	Mapped = pow(Mapped, vec3(1.f / gamma));

	FragColor = vec4(Mapped, 1.f);
#endif
}