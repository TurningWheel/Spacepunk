// Shader.hpp

#pragma once

#include "ArrayList.hpp"
#include "String.hpp"
#include "Asset.hpp"
#include "Main.hpp"

class Shader : public Asset {
public:
	enum shadertype_t {
		VERTEX,
		GEOMETRY,
		FRAGMENT,
		MAX
	};

	Shader();
	Shader(const ArrayList<String>& _defines, shadertype_t _shaderType, const char* _name);
	virtual ~Shader();

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// getters & setters
	virtual const type_t		getType() const			{ return ASSET_SHADER; }
	virtual const shadertype_t	getShaderType() const	{ return shaderType; }
	const GLuint				getShaderObject()		{ return shaderObject; }

private:
	ArrayList<String> defines;
	shadertype_t shaderType = MAX;
	GLuint shaderObject = 0;
	String shaderSource;
	GLint len = 0;

	// runs the below functions
	bool init();

	// loads the shader
	int load();

	// compile the shader
	int compile();
};