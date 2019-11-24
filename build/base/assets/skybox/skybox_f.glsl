#version 330

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec4 Colors;
in vec3 Tangent;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragColorBright;

uniform vec3 gCameraPos;
uniform samplerCube gCubemap;

void main() {
	vec3 lNormal     = normalize(Normal);
	vec3 lWorldPos   = WorldPos;
	vec3 lCameraDir  = normalize(gCameraPos - lWorldPos);
	FragColor = texture(gCubemap, lCameraDir * -1.f);
	
	// check whether fragment output is higher than threshold, if so output to bloom buffer
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        FragColorBright = vec4(FragColor.rgb, 1.f);
    } else {
        FragColorBright = vec4(0.f, 0.f, 0.f, 1.f);
    }
}