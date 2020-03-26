//! @file ShaderProgram.hpp

#pragma once

#include "Asset.hpp"
#include "ArrayList.hpp"
#include "Node.hpp"
#include "Shader.hpp"
#include "Map.hpp"

class Light;

//! A ShaderProgram links multiple Shader objects together into a single program that can be used to render an object
class ShaderProgram : public Asset {
public:
	ShaderProgram();
	ShaderProgram(const char* _name);
	virtual ~ShaderProgram();

	virtual const type_t			getType() const { return ASSET_SHADERPROGRAM; }
	static const ShaderProgram*		getCurrentShader() { return currentShader; }

	//! gets the location of a uniform variable
	//! @param name the name of the variable to be retrieved
	//! @return the location of the uniform with the given name
	GLuint getUniformLocation(const char* name);

	//! binds an attribute to a shader variable
	//! @param index the index of the attribute to assign to the variable
	//! @param name the name of the variable to bind the attribute to
	void bindAttribLocation(GLuint index, const GLchar* name);

	//! uploads light data to the shader, if it hasn't already been uploaded
	//! @param camera The camera that the scene is being drawn from
	//! @param lights The lights to upload
	//! @param maxLights Maximum number of lights that can be uploaded
	//! @param textureUnit First available texture unit for shadow maps
	//! @return The first available texture unit after shadow maps
	Uint32 uploadLights(const Camera& camera, const ArrayList<Light*>& lights, Uint32 maxLights, Uint32 textureUnit);

	//! links all of the shader objects
	//! @return 0 on success, or non-zero on error
	int link();

	//! mounts the shader program into the gl context
	//! @return the shader that was mounted (it might not be this one!)
	ShaderProgram& mount();

	//! unmounts the current shader program from the gl context
	static void unmount();

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	//! quickly build a string that references an indexed uniform array
	//! @param buf the buffer to build the string in
	//! @param name the name of the uniform
	//! @param len number of characters in len (excluding /0)
	//! @param index the offset in the uniform array
	//! @return buf
	static const char* uniformArray(char* buf, const char* name, int len, int index);

private:
	static const ShaderProgram* currentShader;
	ArrayList<Shader> shaders;
	GLuint programObject = 0;
	Map<StringBuf<32>, GLuint> uniforms;
	bool broken = false;
};