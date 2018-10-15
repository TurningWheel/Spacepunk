#version 330

layout(triangles_adjacency) in; // six vertices in
layout(triangle_strip, max_vertices=18) out; // 4 per quad * 3 triangle vertices + 6 for near/far caps

in vec3 Pos[]; // an array of 6 vertices (triangle with adjacency)

uniform vec3 gLightPos;
uniform mat4 gView;

// emit a quad using a triangle strip
void EmitQuad(int startIndex, int endIndex) {
	// vertex #1: the starting vertex (just a tiny bit below the original edge)
	gl_Position = gView * vec4((Pos[startIndex]), 1.0);
	EmitVertex();
 
	// vertex #2: the starting vertex projected to infinity
	vec3 startLightDir = normalize(Pos[startIndex] - gLightPos);
	gl_Position = gView * vec4(startLightDir, 0.0);
	EmitVertex();

	// vertex #3: the ending vertex (just a tiny bit below the original edge)
	gl_Position = gView * vec4((Pos[endIndex]), 1.0);
	EmitVertex();
	
	// vertex #4: the ending vertex projected to infinity
	vec3 endLightDir = normalize(Pos[endIndex] - gLightPos);
	gl_Position = gView * vec4(endLightDir, 0.0);
	EmitVertex();

	EndPrimitive();
}

void main() {
	vec3 e1 = Pos[2] - Pos[0];
	vec3 e2 = Pos[4] - Pos[0];
	vec3 e3 = Pos[1] - Pos[0];
	vec3 e4 = Pos[3] - Pos[2];
	vec3 e5 = Pos[4] - Pos[2];
	vec3 e6 = Pos[5] - Pos[0];

	vec3 normal    = normalize(cross(e1,e2));
	vec3 lightDir  = normalize(gLightPos - Pos[0]);

	// handle only light facing triangles
	if( dot(normal, lightDir) < 0 ) {
		normal = cross(e3,e1);

		if( dot(normal, lightDir) >= 0 ) {
			EmitQuad(0, 2);
		}

		normal = cross(e4,e5);
		lightDir = gLightPos - Pos[2];

		if( dot(normal, lightDir) >= 0 ) {
			EmitQuad(2, 4);
		}

		normal = cross(e2,e6);
		lightDir = gLightPos - Pos[4];

		if( dot(normal, lightDir) >= 0 ) {
			EmitQuad(4, 0);
		}

		// render the front cap
		lightDir = (normalize(Pos[0] - gLightPos));
		gl_Position = gView * vec4((Pos[0]), 1.0);
		EmitVertex();

		lightDir = (normalize(Pos[2] - gLightPos));
		gl_Position = gView * vec4((Pos[2]), 1.0);
		EmitVertex();

		lightDir = (normalize(Pos[4] - gLightPos));
		gl_Position = gView * vec4((Pos[4]), 1.0);
		EmitVertex();
		
		EndPrimitive();
 
		// render the back cap
		lightDir = Pos[0] - gLightPos;
		gl_Position = gView * vec4(lightDir, 0.0);
		EmitVertex();

		lightDir = Pos[4] - gLightPos;
		gl_Position = gView * vec4(lightDir, 0.0);
		EmitVertex();

		lightDir = Pos[2] - gLightPos;
		gl_Position = gView * vec4(lightDir, 0.0);
		EmitVertex();

		EndPrimitive();
	}
}