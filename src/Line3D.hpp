// Line3D.hpp

#pragma once

#include "Main.hpp"

class Camera;

class Line3D {
public:
	Line3D();
	~Line3D();

	// draws the line
	// @param camera: the camera to project the line with
	// @param width: the width of the line to draw, in pixels
	// @param src: the starting point of the line in world space
	// @param dest: the ending point of the line in world space
	// @param color: the line's color
	void draw(Camera& camera, const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color);

private:
	static const GLfloat vertices[6];
	static const GLuint indices[2];

	enum buffer_t {
		VERTEX_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};

	GLuint vbo[BUFFER_TYPE_LENGTH];
	GLuint vao = 0;
};