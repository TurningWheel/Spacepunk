//! @file Shader.hpp

#pragma once

#include "ArrayList.hpp"
#include "String.hpp"
#include "Asset.hpp"
#include "Main.hpp"

//! A Shader contains all the shader code and GL state for a shader (vertex, fragment, whatever)
//! Multiple Shader objects make up a ShaderProgram
class Shader : public Asset {
public:
	enum shadertype_t {
		VERTEX,
		GEOMETRY,
		FRAGMENT,
		MAX
	};

	Shader() = default;
	Shader(const ArrayList<String>& _defines, shadertype_t _shaderType, const char* _name);
	Shader(const Shader&) = delete;
	Shader(Shader&& rhs) {
		Asset::operator=(rhs);
		defines = rhs.defines;
		shaderType = rhs.shaderType;
		shaderObject = rhs.shaderObject;
		shaderSource = rhs.shaderSource;
		rhs.shaderObject = 0;
	}
	virtual ~Shader();

	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&& rhs) {
		Asset::operator=(rhs);
		defines = rhs.defines;
		shaderType = rhs.shaderType;
		shaderObject = rhs.shaderObject;
		shaderSource = rhs.shaderSource;
		rhs.shaderObject = 0;
		return *this;
	}

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	virtual type_t		        getType() const { return ASSET_SHADER; }
	virtual shadertype_t	    getShaderType() const { return shaderType; }
	GLuint				        getShaderObject() { return shaderObject; }

private:
	ArrayList<String> defines;
	shadertype_t shaderType = MAX;
	GLuint shaderObject = 0;
	String shaderSource;

	//! runs the below functions
	bool init();

	//! loads the shader
	int load();

	//! compile the shader
	int compile();
};
