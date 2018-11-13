// ShaderProgram.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"

const ShaderProgram* ShaderProgram::currentShader = nullptr;

ShaderProgram::ShaderProgram() : Asset() {
	programObject = glCreateProgram();
}

ShaderProgram::ShaderProgram(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();
	programObject = glCreateProgram();
	loaded = FileHelper::readObject(path.get(), *this);
}

ShaderProgram::~ShaderProgram() {
	if( programObject ) {
		for( auto shader : shaders ) {
			glDetachShader(programObject,shader.getShaderObject());
		}
		shaders.clear();
		glDeleteProgram(programObject);
	}
}

const GLuint ShaderProgram::getUniformLocation(const char* name) const {
	return glGetUniformLocation(programObject,(GLchar*)name);
}

void ShaderProgram::bindAttribLocation(GLuint index, const GLchar* name) {
	glBindAttribLocation(programObject,index,name);
}

int ShaderProgram::link() {
	GLint linked;
	glLinkProgram(programObject);
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if( linked ) {
		// successfully linked
		mainEngine->msg(Engine::MSG_DEBUG,"linked shader program successfully");
		return 0;
	} else {
		// show error message
		GLint blen = 0;
		GLsizei slen = 0;

		glGetShaderiv(programObject, GL_INFO_LOG_LENGTH, &blen);
		if( blen > 1 ) {
			GLchar* linkLog = new GLchar[blen+1];
			if( linkLog ) {
				linkLog[blen] = 0;
				glGetProgramInfoLog(programObject, blen, &slen, linkLog);
				linkLog[blen] = 0;
				mainEngine->fmsg(Engine::MSG_ERROR,"failed to link shader program: %s",linkLog);
				delete[] linkLog;
			}
		}

		return 1;
	}
}

void ShaderProgram::mount() const {
	glUseProgram(programObject);
	currentShader = this;
}

void ShaderProgram::unmount() {
	glUseProgram(0);
	currentShader = nullptr;
}

void ShaderProgram::serialize(FileInterface* file) {
	int version = 0;
	file->property("ShaderProgram::version", version);
	file->property("shaders", shaders);
	if (file->isReading()) {
		for (auto& shader : shaders) {
			glAttachShader(programObject,shader.getShaderObject());
		}
		loaded = (link() == 0);
	}
}