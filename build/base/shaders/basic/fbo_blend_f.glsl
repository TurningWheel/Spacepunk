#version 400

out vec4 FragColor;

in vec2 TexCoord;

uniform ivec2 gResolution;
#ifdef MULTISAMPLE
uniform sampler2DMS gTexture0;
uniform sampler2DMS gTexture1;
#else
uniform sampler2D gTexture0;
uniform sampler2D gTexture1;
#endif

void main() {
#ifdef MULTISAMPLE
    vec4 Color0 = vec4(0.f);
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 0) * 0.25f;
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 1) * 0.25f;
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 2) * 0.25f;
    Color0 += texelFetch(gTexture0, ivec2(TexCoord * gResolution), 3) * 0.25f;

    vec4 Color1 = vec4(0.f);
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 0) * 0.25f;
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 1) * 0.25f;
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 2) * 0.25f;
    Color1 += texelFetch(gTexture1, ivec2(TexCoord * gResolution), 3) * 0.25f;
#else
    vec4 Color0 = texelFetch(gTexture0, ivec2(TexCoord * gResolution), 0);
    vec4 Color1 = texelFetch(gTexture1, ivec2(TexCoord * gResolution), 0);
#endif

    FragColor = vec4(Color0.rgb + Color1.rgb, Color0.a);
}