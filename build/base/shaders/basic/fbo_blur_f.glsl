#version 400

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2DMS gTexture;
uniform ivec2 gResolution;

vec3 getColor(vec2 coords) {
    vec3 Color = vec3(0.f);
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 0).rgb * 0.25f;
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 1).rgb * 0.25f;
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 2).rgb * 0.25f;
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 3).rgb * 0.25f;
    return Color;
}

void main() {
    const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = getColor(TexCoord) * weight[0]; // current fragment's contribution
#ifdef HORIZONTAL
    for(int i = 1; i < 5; ++i) {
        float offset = float(i) / float(gResolution.x);
        result += getColor(TexCoord + vec2(offset, 0.f)) * weight[i];
        result += getColor(TexCoord - vec2(offset, 0.f)) * weight[i];
    }
#else
    for(int i = 1; i < 5; ++i) {
        float offset = float(i) / float(gResolution.y);
        result += getColor(TexCoord + vec2(0.f, offset)) * weight[i];
        result += getColor(TexCoord - vec2(0.f, offset)) * weight[i];
    }
#endif
    FragColor = vec4(result, 1.f);
}