// ShaderProgram.hpp

#pragma once

#include "Asset.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Shader.hpp"

class ShaderProgram : public Asset {
public:
	ShaderProgram() {}
	ShaderProgram(const char* _name);
	virtual ~ShaderProgram();

	// getters & setters
	virtual const type_t			getType() const		{ return ASSET_SHADERPROGRAM; }
	static const ShaderProgram*		getCurrentShader()	{ return currentShader; }

	// adds a shader to the current program
	// @param filename: the filename of the shader source
	// @return 0 on success, or non-zero on error
	int addShader(const char* filename);

	// removes a shader from the given program
	// @param index: the index of the shader to be removed from the program
	// @return 0 on success, or non-zero on error
	int removeShader(const Uint32 index);

	// gets the location of a uniform variable
	// @param name: the name of the variable to be retrieved
	// @return the location of the uniform with the given name
	const GLuint getUniformLocation(const char* name) const;

	// binds an attribute to a shader variable
	// @param index: the index of the attribute to assign to the variable
	// @param name: the name of the variable to bind the attribute to
	void bindAttribLocation(GLuint index, const GLchar* name);

	// links all of the shader objects
	// @return 0 on success, or non-zero on error
	int link();

	// mounts the shader program into the gl context
	void mount() const;

	// unmounts the current shader program from the gl context
	static void unmount();

private:
	LinkedList<Shader*> shaders;
	GLuint programObject = 0;
	static const ShaderProgram* currentShader;
};