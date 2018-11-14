// Shader.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Shader.hpp"

Shader::Shader() : Asset() {

}

Shader::Shader(const ArrayList<String>& _defines, shadertype_t _shaderType, const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();
	shaderType = _shaderType;
	defines = _defines;
	loaded = init();
}

Shader::~Shader() {
	if( shaderObject )
		glDeleteShader(shaderObject);
}

bool Shader::init() {
	// load shader source
	int error=0;
	if( (error=load()) != 0 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load shader '%s': error code (%d)", name.get(), error);
		return false;
	}
	if( (error=compile()) != 0 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to compile shader '%s': error code (%d)", name.get(), error);
		return false;
	}
	return true;
}

int Shader::load() {
	FILE* fp;

	// open file
	fp = fopen(path.get(),"r");
	if( fp==NULL )
		return 1; // file not found
	clearerr(fp);

	// length of preproc defines
	static const char* definePrefix = "#define ";
	static const char* defineSuffix = "\n";
	GLint definesLength = 0;
	GLint definePrefixLen = (GLint)strlen(definePrefix);
	GLint defineSuffixLen = (GLint)strlen(defineSuffix);
	for (auto& define : defines) {
		definesLength += definePrefixLen + (GLint)define.length() + defineSuffixLen;
	}

	// determine length of file
	size_t len = 0;
	for( ; !feof(fp); ++len )
		fgetc(fp);

	// reset to beginning of file
	fseek(fp,0,SEEK_SET);
	clearerr(fp);

	// error if empty file
	if( len<=0 ) {
		fclose(fp);
		return 2; // empty file
	}

	// allocate memory for source
	shaderSource.alloc(len+definesLength+1);
	shaderSource[len+definesLength] = '\0';
	int i = 0;

	// read version directive from source
	if (defines.getSize() > 0) {
		for( ; !feof(fp); ++i ) {
			GLchar c = (GLchar)fgetc(fp);
			shaderSource[i] = c;
			if (c == '\n') {
				++i;
				shaderSource[i] = '\0';
				break;
			}
		}
	}

	// insert defines
	for (auto& define : defines) {
		shaderSource.append(definePrefix);
		shaderSource.append(define.get());
		shaderSource.append(defineSuffix);
		i += (int)definePrefixLen;
		i += (int)define.length();
		i += (int)defineSuffixLen;
	}

	// read rest of shader source
	for( ; !feof(fp); ++i )
		shaderSource[i] = (GLchar)fgetc(fp);
	shaderSource[i] = '\0';
	shaderSource[i-1] = '\0';

	// store off final length
	this->len = (GLint)shaderSource.getSize() - 1;

	// close file, return success
	fclose(fp);
	return 0;
}

int Shader::compile() {
	GLenum glShaderType = GL_GEOMETRY_SHADER;

	// determine shader type
	switch( shaderType ) {
		case VERTEX:
			glShaderType = GL_VERTEX_SHADER;
			break;
		case GEOMETRY:
			glShaderType = GL_GEOMETRY_SHADER;
			break;
		case FRAGMENT:
			glShaderType = GL_FRAGMENT_SHADER;
			break;
		default:
			mainEngine->fmsg(Engine::MSG_ERROR,"attempted to load shader of invalid type: '%s'",name.get());
			return 1;
	}

	// create shader object
	shaderObject = glCreateShader(glShaderType);
	const GLchar* src = shaderSource.get();
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
		return 2;
	}

	return 0;
}

void Shader::serialize(FileInterface* file) {
	int version = 0;
	file->property("Shader::version", version);
	file->property("type", shaderType);
	file->property("path", name);
	file->property("defines", defines);
	if (file->isReading()) {
		path = mainEngine->buildPath(name.get()).get();
		loaded = init();
	}
}