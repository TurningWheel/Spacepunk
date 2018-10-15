// Shader.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Shader.hpp"

Shader::Shader(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();

	// load shader source
	int error=0;
	if( (error=load()) != 0 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load shader '%s': error code (%d)", name.get(), error);
		return;
	}

	GLenum shaderType = GL_GEOMETRY_SHADER;
	switch( name.get()[name.length()-2] ) {
	case 'g':
		shaderType = GL_GEOMETRY_SHADER;
		break;
	case 'v':
		shaderType = GL_VERTEX_SHADER;
		break;
	case 'f':
		shaderType = GL_FRAGMENT_SHADER;
		break;
	default:
		mainEngine->fmsg(Engine::MSG_ERROR,"attempted to load shader '%s', which is neither *.gs, *.vs, *.fs in extension",name.get());
		return;
	}

	// create shader object
	shaderObject = glCreateShader(shaderType);
	const GLchar* src = (const GLchar*)shaderSource;
	glShaderSource(shaderObject, 1, &src, &len);

	// compile shader
	GLint compiled;
	glCompileShader(shaderObject);
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &compiled);
	if( compiled ) {
		// successfully compiled
		mainEngine->fmsg(Engine::MSG_DEBUG,"compiled shader '%s'",name.get());
	} else {
		// show error message
		GLint blen = 0;
		GLsizei slen = 0;

		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &blen);
		if( blen > 1 ) {
			GLchar* compilerLog = new GLchar[blen+1];
			if( compilerLog ) {
				compilerLog[blen] = 0;
				glGetShaderInfoLog(shaderObject, blen, &slen, compilerLog);
				compilerLog[blen] = 0;
				mainEngine->fmsg(Engine::MSG_ERROR,"failed to compile shader '%s': %s",name.get(),compilerLog);
				delete[] compilerLog;
			}
		}
		return;
	}

	// success
	loaded = true;
}

Shader::~Shader() {
	if( shaderObject )
		glDeleteShader(shaderObject);
	if( shaderSource )
		delete[] shaderSource;
	shaderSource = 0;
}

int Shader::load() {
	FILE* fp;

	fp = fopen(path.get(),"r");
	if( fp==NULL )
		return 1; // file not found
	clearerr(fp);

	// get length of file
	for( len=0; !feof(fp); ++len )
		fgetc(fp);
	fseek(fp,0,SEEK_SET);
	clearerr(fp);

	if( len<=0 ) {
		fclose(fp);
		return 2; // empty file
	}

	shaderSource = new GLchar[len+1];
	if( shaderSource == nullptr )
		return 3; // no memory left
	shaderSource[len] = 0;

	int i;
	for( i=0; !feof(fp); ++i )
		shaderSource[i] = (GLchar)fgetc(fp);
	shaderSource[i] = 0;
	shaderSource[i-1] = 0;

	fclose(fp);
	return 0;
}