#version 430

in vec2 TexCoord;

out vec4 FragColor;

#ifdef MULTISAMPLE
uniform sampler2DMS gTexture;
#else
uniform sampler2D gTexture;
#endif
uniform ivec2 gResolution;
uniform float gGamma;

vec4 tex(ivec2 coords) {
#ifdef MULTISAMPLE
	vec4 Color = vec4(0.f);
	Color += texelFetch(gTexture, coords, 0) * 0.25f;
	Color += texelFetch(gTexture, coords, 1) * 0.25f;
	Color += texelFetch(gTexture, coords, 2) * 0.25f;
	Color += texelFetch(gTexture, coords, 3) * 0.25f;
#else
	vec4 Color = texelFetch(gTexture, coords, 0);
#endif
	return Color;
}

#ifdef GUI
float Triangular(float f) {
	f = f / 2.f;
	if (f < 0.f) {
		return f + 1.f;
	} else {
		return 1.f - f;
	}
	return 0.f;
}

vec4 BiCubic(vec2 coords, vec2 res1, vec2 res2) {
    vec4 nSum   = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 nDenom = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float a = fract(coords.x); // get the decimal part
    float b = fract(coords.y); // get the decimal part
    ivec2 samples = ivec2(ceil(res1.x / res2.x),
    	ceil(res1.y / res2.y));
    ivec2 start = - samples / 2;
    ivec2 end = samples / 2;
    for (int m = start.x; m <= end.x; ++m) {
        for (int n = start.y; n <= end.y; ++n) {
			vec4 vecData = tex(ivec2(coords + vec2(float(m), float(n))));
			float f0 = Triangular(float(m) - a);
			vec4 vecCoef0 = vec4(f0, f0, f0, f0);
			float f1 = Triangular(-(float(n) - b));
			vec4 vecCoef1 = vec4(f1, f1, f1, f1);
            nSum = nSum + (vecData * vecCoef0 * vecCoef1);
            if (vecData.a > 0.f) {
            	nDenom = nDenom + (vecCoef0 * vecCoef1);
            }
        }
    }
    return nSum / nDenom;
}
#endif

void main() {
#ifndef HDR
#ifndef GUI
	// basic copy
	vec4 Color = tex(ivec2(TexCoord * gResolution));
#ifndef GAMMA
	FragColor = Color;
#else
	FragColor = vec4(Color.rgb * gGamma, min(Color.a, 1.0));
#endif
#else
	// gui
	vec2 guiRes = vec2(1920.f, 1080.f);
	vec2 scrRes = vec2(gResolution);
	vec2 res1 = vec2(guiRes.x > scrRes.x ? guiRes.x : scrRes.x, guiRes.y > scrRes.y ? guiRes.y : scrRes.y);
	vec2 res2 = vec2(guiRes.x > scrRes.x ? scrRes.x : guiRes.x, guiRes.y > scrRes.y ? scrRes.y : guiRes.y);
	vec4 Color = BiCubic(TexCoord * guiRes, res1, res2);
	FragColor = Color;
#endif
#else
	// HDR tone-mapping
	vec3 Color = tex(ivec2(TexCoord * gResolution)).rgb;

	// reinhard tone mapping
	//vec3 Mapped = Color / (Color + vec3(1.f));

	// exposure tone mapping
	const float exposure = 1.5f;
	vec3 Mapped = vec3(1.f) - exp(-Color * exposure);

	// gamma correction
	const float gamma = 1.f;
	Mapped = pow(Mapped, vec3(1.f / gamma));

	FragColor = vec4(Mapped, 1.f);
#endif
	FragColor = clamp(FragColor, vec4(0.f), vec4(1.f));
}