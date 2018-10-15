// ShaderProgram.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"

const ShaderProgram* ShaderProgram::currentShader = nullptr;

ShaderProgram::ShaderProgram(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();

	programObject = glCreateProgram();
	loaded = true;
}

ShaderProgram::~ShaderProgram() {
	if( programObject ) {
		Node<Shader*>* nextNode = nullptr;
		for( Node<Shader*>* node=shaders.getFirst(); node!=nullptr; node=nextNode ) {
			nextNode = node->getNext();
			Shader* shader = node->getData();
			glDetachShader(programObject,shader->getShaderObject());
			shaders.removeNode(node);
			delete shader;
		}
		glDeleteProgram(programObject);
	}
}

int ShaderProgram::addShader(const char* filename) {
	Shader* shader = new Shader(filename);
	if( !shader ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to add shader '%s' to program '%s'", filename, name.get());
		return 1; // failed to create shader
	}
	
	if( !shader->isLoaded() ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to init shader '%s' in program '%s'", filename, name.get());
		return 2; // failed to initialized shader
	}

	shaders.addNodeLast(shader);
	glAttachShader(programObject,shader->getShaderObject());

	return 0;
}

int ShaderProgram::removeShader(const Uint32 index) {
	Node<Shader*>* node = shaders.nodeForIndex(index);
	if( node ) {
		Shader* shader = node->getData();
		glDetachShader(programObject,shader->getShaderObject());
		shaders.removeNode(node);
		delete shader;
		return 0;
	} else {
		return 1;
	}
}

const GLuint ShaderProgram::getUniformLocation(const char* name) const {
	return glGetUniformLocation(programObject,(GLchar*)name);
}

void ShaderProgram::bindAttribLocation(GLuint index, const GLchar* name) {
	glBindAttribLocation(programObject,index,name);
}

int ShaderProgram::link() {
	if( shaders.getSize()==0 ) {
		mainEngine->fmsg(Engine::MSG_WARN,"no shaders to link for program '%s'",name.get());
		return 1;
	}

	GLint linked;
	glLinkProgram(programObject);
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if( linked ) {
		// successfully linked
		mainEngine->fmsg(Engine::MSG_DEBUG,"linked shader program '%s'",name.get());

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
				mainEngine->fmsg(Engine::MSG_ERROR,"failed to link shader program '%s': %s",name.get(),linkLog);
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