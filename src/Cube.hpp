//! @file Cube.hpp

#pragma once

#include "Main.hpp"

class Camera;

class Cube {
public:
	Cube();
	~Cube();

	//! draws the cube
	//! @param camera the camera to project the cube with
	//! @param transform the transformation to apply to the cube
	//! @param color the cube's color
	void draw(Camera& camera, const glm::mat4& transform, const glm::vec4& color);

private:
	static const GLfloat vertices[72];
	static const GLfloat texcoord[48];
	static const GLfloat normals[72];
	static const GLuint indices[36];

	enum buffer_t {
		VERTEX_BUFFER,
		TEXCOORD_BUFFER,
		NORMAL_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};

	GLuint vbo[BUFFER_TYPE_LENGTH];
	GLuint vao = 0;
};