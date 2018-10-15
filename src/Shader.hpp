// Shader.hpp

#pragma once

#include "Asset.hpp"
#include "Main.hpp"

class Shader : public Asset {
public:
	Shader() {}
	Shader(const char* _name);
	virtual ~Shader();

	// getters & setters
	virtual const type_t	getType() const		{ return ASSET_SHADER; }
	const GLuint			getShaderObject()	{ return shaderObject; }

private:
	GLchar* shaderSource = nullptr;
	GLint len = 0;
	GLuint shaderObject = 0;

	// loads the shader
	int load();
};