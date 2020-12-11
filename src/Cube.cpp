// Cube.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"
#include "Renderer.hpp"
#include "Cube.hpp"

const GLfloat Cube::vertices[72] = {
	0.5, -.5, 0.5,
	0.5, 0.5, 0.5,
	-.5, 0.5, 0.5,
	-.5, -.5, 0.5,

	-.5, -.5, 0.5,
	-.5, 0.5, 0.5,
	-.5, 0.5, -.5,
	-.5, -.5, -.5,

	0.5, 0.5, 0.5,
	0.5, -.5, 0.5,
	0.5, -.5, -.5,
	0.5, 0.5, -.5,

	0.5, 0.5, -.5,
	0.5, -.5, -.5,
	-.5, -.5, -.5,
	-.5, 0.5, -.5,

	-.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, -.5,
	-.5, 0.5, -.5,

	0.5, -.5, -.5,
	0.5, -.5, 0.5,
	-.5, -.5, 0.5,
	-.5, -.5, -.5
};

const GLfloat Cube::texcoord[48] = {
	0, 0,
	0, 1,
	1, 1,
	1, 0,

	0, 0,
	0, 1,
	1, 1,
	1, 0,

	0, 0,
	0, 1,
	1, 1,
	1, 0,

	0, 0,
	0, 1,
	1, 1,
	1, 0,

	0, 0,
	0, 1,
	1, 1,
	1, 0,

	0, 0,
	0, 1,
	1, 1,
	1, 0
};

const GLfloat Cube::normals[72] = {
	0, 0, 1,
	0, 0, 1,
	0, 0, 1,
	0, 0, 1,

	-1, 0, 0,
	-1, 0, 0,
	-1, 0, 0,
	-1, 0, 0,

	1, 0, 0,
	1, 0, 0,
	1, 0, 0,
	1, 0, 0,

	0, 0, -1,
	0, 0, -1,
	0, 0, -1,
	0, 0, -1,

	0, 1, 0,
	0, 1, 0,
	0, 1, 0,
	0, 1, 0,

	0, -1, 0,
	0, -1, 0,
	0, -1, 0,
	0, -1, 0
};

const GLuint Cube::indices[36]{
	0, 1, 2,
	0, 2, 3,

	4, 5, 6,
	4, 6, 7,

	8, 9, 10,
	8, 10, 11,

	12, 13, 14,
	12, 14, 15,

	16, 17, 18,
	16, 18, 19,

	20, 21, 22,
	20, 22, 23
};

Cube::Cube() {
	// initialize buffer names
	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	// create vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// upload vertex data
	glGenBuffers(1, &vbo[VERTEX_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 24 * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	// upload texcoord data
	glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 24 * 2 * sizeof(GLfloat), texcoord, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	// upload normal data
	glGenBuffers(1, &vbo[NORMAL_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 24 * 3 * sizeof(GLfloat), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(2);

	// upload index data
	glGenBuffers(1, &vbo[INDEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), indices, GL_STATIC_DRAW);

	// unbind vertex array
	glBindVertexArray(0);
}

Cube::~Cube() {
	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		buffer_t buffer = static_cast<buffer_t>(i);
		if (vbo[buffer]) {
			glDeleteBuffers(1, &vbo[buffer]);
		}
	}
	if (vao) {
		glDeleteVertexArrays(1, &vao);
	}
}

void Cube::draw(Camera& camera, const glm::mat4& transform, const glm::vec4& color) {
	// setup view matrix
	glm::mat4 viewMatrix;
	if (camera.getDrawMode() == Camera::DRAW_BOUNDS &&
		camera.getEntity()->getName() != "Shadow Camera") {
		// TODO fix this ugly hack!
		Vector diff = Vector(transform[3][0], transform[3][2], -transform[3][1]) - camera.getGlobalPos();
		Quaternion ang = Rotation(atan2f(diff.y, diff.x), atan2f(diff.z, sqrtf(diff.x*diff.x + diff.y*diff.y)), 0.f);
		Quaternion q = Quaternion(Rotation(PI / 2.f, 0.f, 0.f)) * ang;
		glm::mat4 rot = glm::mat4(glm::quat(-q.w, q.z, q.y, -q.x));
		glm::mat4 pos = glm::translate(glm::mat4(1.f), glm::vec3(
			-camera.getGlobalPos().x, camera.getGlobalPos().z, -camera.getGlobalPos().y));
		glm::mat4 final = rot * pos;
		viewMatrix = camera.getProjMatrix() * final * transform;
	} else {
		viewMatrix = camera.getProjViewMatrix() * transform;
	}

	// load shader
	Material* mat = mainEngine->getMaterialResource().dataForString("shaders/basic/cube.json");
	if (mat) {
		ShaderProgram& shader = mat->getShader().mount();

		// upload uniform variables
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniform4fv(shader.getUniformLocation("gColor"), 1, glm::value_ptr(color));

		// draw elements
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
	}
}