#version 400

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2DMS gTexture;
uniform ivec2 gResolution;

#define PI 3.14159265358979323846
#define  E 2.71828182845904523536

float gauss(float off, float dev) {
    float off2 = off * off;
    float dev2 = dev * dev;
    return (1.f / sqrt(2.f * PI * dev2)) * pow(E, -((off2) / (2.f * dev2)));
}

vec3 getColor(vec2 coords) {
    vec3 Color = vec3(0.f);
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 0).rgb * 0.25f;
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 1).rgb * 0.25f;
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 2).rgb * 0.25f;
    Color += texelFetch(gTexture, ivec2(coords * gResolution), 3).rgb * 0.25f;
    return Color;
}

void main() {
    float dev        = 1.f;
    vec3  result     = getColor(TexCoord) * gauss(0.f, dev); // current fragment's contribution

#ifdef HORIZONTAL
    for(int i = 1; i < 10; ++i) {
        float offset = float(i) / float(gResolution.x);
        result += getColor(TexCoord + vec2(offset, 0.f)) * gauss(float(i), dev);
        result += getColor(TexCoord - vec2(offset, 0.f)) * gauss(float(i), dev);
    }
#else
    for(int i = 1; i < 10; ++i) {
        float offset = float(i) / float(gResolution.y);
        result += getColor(TexCoord + vec2(0.f, offset)) * gauss(float(i), dev);
        result += getColor(TexCoord - vec2(0.f, offset)) * gauss(float(i), dev);
    }
#endif

    FragColor = vec4(result, 1.f);
}