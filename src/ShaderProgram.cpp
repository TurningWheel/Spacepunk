// ShaderProgram.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"
#include "Light.hpp"
#include "Camera.hpp"

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

void ShaderProgram::mount() {
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

void ShaderProgram::uploadLights(const Camera& camera, const ArrayList<Light*>& lights, Uint32 maxLights, Uint32 textureUnit) {
	if (lastFrameDrawn == camera.getFramesDrawn()) {
		return;
	} else {
		lastFrameDrawn = camera.getFramesDrawn();
	}

	Uint32 index = 0;
	StringBuf<32> buf;
	for (auto light : lights) {
		Vector lightAng = light->getGlobalAng().toVector();
		glm::vec3 lightDir( lightAng.x, -lightAng.z, lightAng.y );
		glm::vec3 lightPos( light->getGlobalPos().x, -light->getGlobalPos().z, light->getGlobalPos().y );
		glm::vec3 lightScale( light->getGlobalScale().x, -light->getGlobalScale().z, light->getGlobalScale().y );

		glUniform3fv(getUniformLocation(buf.format("gLightPos[%d]",index)), 1, glm::value_ptr(lightPos));
		glUniform4fv(getUniformLocation(buf.format("gLightColor[%d]",index)), 1, glm::value_ptr(glm::vec3(light->getColor())));
		glUniform1f(getUniformLocation(buf.format("gLightIntensity[%d]",index)), light->getIntensity());
		glUniform1f(getUniformLocation(buf.format("gLightRadius[%d]",index)), light->getRadius());
		glUniform1f(getUniformLocation(buf.format("gLightArc[%d]",index)), light->getArc() * PI / 180.f);
		glUniform3fv(getUniformLocation(buf.format("gLightScale[%d]",index)), 1, glm::value_ptr(lightScale));
		glUniform3fv(getUniformLocation(buf.format("gLightDirection[%d]",index)), 1, glm::value_ptr(lightDir));
		glUniform1i(getUniformLocation(buf.format("gLightShape[%d]",index)), static_cast<GLint>(light->getShape()));
		if (light->isShadow() && light->getEntity()->isFlag(Entity::FLAG_SHADOW)) {
			glUniform1i(getUniformLocation(buf.format("gShadowmap[%d]",(int)index)), textureUnit);
			glUniform1i(getUniformLocation(buf.format("gShadowmapEnabled[%d]",(int)(index))), GL_TRUE);
			light->getShadowMap().bindForReading(GL_TEXTURE0+textureUnit);
			glm::mat4 lightProj = glm::perspective( glm::radians(90.f), 1.f, 1.f, light->getRadius() );
			glUniformMatrix4fv(getUniformLocation(buf.format("gLightProj[%d]",(int)(index))), 1, GL_FALSE, glm::value_ptr(lightProj));
		} else {
			glUniform1i(getUniformLocation(buf.format("gShadowmapEnabled[%d]",(int)(index))), GL_FALSE);
		}

		++index;
		++textureUnit;
		if (index >= maxLights || textureUnit >= GL_MAX_TEXTURE_IMAGE_UNITS) {
			break;
		}
	}
	glUniform1i(getUniformLocation("gNumLights"), (GLint)lights.getSize());

	// upload light matrix
	glm::mat4 lightProjMatrix = glm::perspective( glm::radians(90.f), 1.f, Shadow::camerainfo_t::clipNear, Shadow::camerainfo_t::clipFar );
	glUniform4fv(getUniformLocation("gLightProj"), 1, glm::value_ptr(lightProjMatrix));
}