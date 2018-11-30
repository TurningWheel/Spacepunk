// ShaderProgram.hpp

#pragma once

#include "Asset.hpp"
#include "ArrayList.hpp"
#include "Node.hpp"
#include "Shader.hpp"

class Light;

class ShaderProgram : public Asset {
public:
	ShaderProgram();
	ShaderProgram(const char* _name);
	virtual ~ShaderProgram();

	// getters & setters
	virtual const type_t			getType() const		{ return ASSET_SHADERPROGRAM; }
	static const ShaderProgram*		getCurrentShader()	{ return currentShader; }

	// gets the location of a uniform variable
	// @param name the name of the variable to be retrieved
	// @return the location of the uniform with the given name
	const GLuint getUniformLocation(const char* name) const;

	// binds an attribute to a shader variable
	// @param index the index of the attribute to assign to the variable
	// @param name the name of the variable to bind the attribute to
	void bindAttribLocation(GLuint index, const GLchar* name);

	// uploads light data to the shader, if it hasn't already been uploaded
	// @param camera The camera that the scene is being drawn from
	// @param lights The lights to upload
	// @param maxLights Maximum number of lights that can be uploaded
	void uploadLights(const Camera& camera, const ArrayList<Light*>& lights, Uint32 maxLights);

	// links all of the shader objects
	// @return 0 on success, or non-zero on error
	int link();

	// mounts the shader program into the gl context
	void mount();

	// unmounts the current shader program from the gl context
	static void unmount();

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

private:
	static const ShaderProgram* currentShader;
	ArrayList<Shader> shaders;
	GLuint programObject = 0;
	Uint32 lastFrameDrawn = UINT32_MAX;
};