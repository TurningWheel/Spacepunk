#version 400

out vec4 FragColor;

in vec2 TexCoord;

uniform ivec2 gResolution;
uniform sampler2DMS gTexture0;
uniform sampler2DMS gTexture1;

void main() {
    vec3 Color0 = vec3(0.f);
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 0).rgb * 0.25f;
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 1).rgb * 0.25f;
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 2).rgb * 0.25f;
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 3).rgb * 0.25f;

    vec3 Color1 = vec3(0.f);
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 0).rgb * 0.25f;
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 1).rgb * 0.25f;
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 2).rgb * 0.25f;
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 3).rgb * 0.25f;

    FragColor = vec4(Color0 + Color1, 1.f);
}