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
	if (programObject) {
		for (auto& shader : shaders) {
			glDetachShader(programObject, shader.getShaderObject());
		}
		shaders.clear();
		glDeleteProgram(programObject);
	}
}

GLuint ShaderProgram::getUniformLocation(const char* name) {
	GLuint* uniform = uniforms[name];
	if (uniform == nullptr) {
		GLuint value = glGetUniformLocation(programObject, (GLchar*)name);
		uniforms.insert(name, value);
		return value;
	} else {
		return *uniform;
	}
}

void ShaderProgram::bindAttribLocation(GLuint index, const GLchar* name) {
	glBindAttribLocation(programObject, index, name);
}

int ShaderProgram::link() {
	GLint linked;
	glLinkProgram(programObject);
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if (linked) {
		// successfully linked
		mainEngine->msg(Engine::MSG_DEBUG, "linked shader program successfully");
		broken = false;
		return 0;
	} else {
		// show error message
		GLint blen = 0;
		GLsizei slen = 0;

		glGetShaderiv(programObject, GL_INFO_LOG_LENGTH, &blen);
		if (blen > 1) {
			GLchar* linkLog = new GLchar[blen + 1];
			if (linkLog) {
				linkLog[blen] = 0;
				glGetProgramInfoLog(programObject, blen, &slen, linkLog);
				linkLog[blen] = 0;
				mainEngine->fmsg(Engine::MSG_ERROR, "failed to link shader program: %s", linkLog);
				delete[] linkLog;
			}
		}

		broken = true;
		return 1;
	}
	uniforms.clear();
}

ShaderProgram& ShaderProgram::mount() {
	if (currentShader == this) {
		return *this;
	} else {
		if (!broken && loaded && shaders.getSize() > 0) {
			glUseProgram(programObject);
			currentShader = this;
			return *this;
		} else {
			Material* mat = mainEngine->getMaterialResource().dataForString("shaders/actor/error.json");
			assert(&mat->getShader() != this);
			return mat->getShader().mount();
		}
	}
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
			glAttachShader(programObject, shader.getShaderObject());
		}
		loaded = (link() == 0);
	}
}

const char* ShaderProgram::uniformArray(char* buf, const char* name, int len, int index) {
	strcpy(buf, name);
	buf[len] = '[';
	if (index == 0) {
		buf[len + 1] = '0';
		buf[len + 2] = ']';
		buf[len + 3] = '\0';
	} else {
		static ArrayList<char> chars;
		unsigned int a = index;
		while (a != 0) {
			chars.push('0' + a % 10);
			a /= 10;
		}
		++len;
		while (chars.getSize()) {
			buf[len] = chars.pop();
			++len;
		}
		buf[len] = ']';
		buf[len + 1] = '\0';
	}
	return buf;
}

Uint32 ShaderProgram::uploadLights(const Camera& camera, const ArrayList<Light*>& lights, Uint32 maxLights, Uint32 textureUnit) {
	char buf[32];
	Uint32 index = 0;
	for (auto light : lights) {
		Vector lightAng = light->getGlobalAng().toVector();
		glm::vec3 lightDir(lightAng.x, -lightAng.z, lightAng.y);
		glm::vec3 lightPos(light->getGlobalPos().x, -light->getGlobalPos().z, light->getGlobalPos().y);
		glm::vec3 lightScale(light->getGlobalScale().x, -light->getGlobalScale().z, light->getGlobalScale().y);

		glUniform3fv(getUniformLocation(uniformArray(buf, "gLightPos", 9, index)), 1, glm::value_ptr(lightPos));
		glUniform3fv(getUniformLocation(uniformArray(buf, "gLightColor", 11, index)), 1, glm::value_ptr(glm::vec3(light->getColor())));
		glUniform1f(getUniformLocation(uniformArray(buf, "gLightIntensity", 15, index)), light->getIntensity());
		glUniform1f(getUniformLocation(uniformArray(buf, "gLightRadius", 12, index)), light->getRadius());
		glUniform1f(getUniformLocation(uniformArray(buf, "gLightArc", 9, index)), light->getArc() * PI / 180.f);
		glUniform3fv(getUniformLocation(uniformArray(buf, "gLightScale", 11, index)), 1, glm::value_ptr(lightScale));
		glUniform3fv(getUniformLocation(uniformArray(buf, "gLightDirection", 15, index)), 1, glm::value_ptr(lightDir));
		glUniform1i(getUniformLocation(uniformArray(buf, "gLightShape", 11, index)), static_cast<GLint>(light->getShape()));
		if (light->isShadow() && light->getEntity()->isFlag(Entity::FLAG_SHADOW) && cvar_shadowsEnabled.toInt()) {
			glUniform1i(getUniformLocation(uniformArray(buf, "gShadowmapEnabled", 17, index)), GL_TRUE);
			glUniform1i(getUniformLocation(uniformArray(buf, "gShadowmap", 10, index)), textureUnit);
			light->getShadowMap().bindForReading(GL_TEXTURE0 + textureUnit, GL_DEPTH_ATTACHMENT);
			//glm::mat4 lightProj = glm::perspective( glm::radians(90.f), 1.f, 1.f, light->getRadius() );
			glm::mat4 lightProj = Camera::makeInfReversedZProj(glm::radians(90.f), 1.f, 1.f);
			glUniformMatrix4fv(getUniformLocation(uniformArray(buf, "gLightProj", 10, index)), 1, GL_FALSE, glm::value_ptr(lightProj));
		} else {
			glUniform1i(getUniformLocation(uniformArray(buf, "gShadowmapEnabled", 17, index)), GL_FALSE);
			glUniform1i(getUniformLocation(uniformArray(buf, "gShadowmap", 10, index)), textureUnit);
			camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_DEPTH_ATTACHMENT);
		}

		++index;
		++textureUnit;
		if (index >= maxLights) {
			break;
		}
	}
	for (; index < maxLights; ++index) {
		glUniform1i(getUniformLocation(uniformArray(buf, "gShadowmapEnabled", 17, index)), GL_FALSE);
		glUniform1i(getUniformLocation(uniformArray(buf, "gShadowmap", 10, index)), textureUnit);
		camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_DEPTH_ATTACHMENT);
		++textureUnit;
	}

	index = 0u;
	for (auto light : lights) {
		/*if (light->isShadow() && light->getEntity()->isFlag(Entity::FLAG_SHADOW) && cvar_shadowsEnabled.toInt()) {
			glUniform1i(getUniformLocation(uniformArray(buf, "gUIDmap", 7, index)), textureUnit);
			light->getShadowMap().bindForReading(GL_TEXTURE0 + textureUnit, GL_COLOR_ATTACHMENT0);
		} else {
			glUniform1i(getUniformLocation(uniformArray(buf, "gUIDmap", 7, index)), textureUnit);
			camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_COLOR_ATTACHMENT0);
		}*/

		++index;
		++textureUnit;
		if (index >= maxLights) {
			break;
		}
	}
	/*for (; index < maxLights; ++index) {
		glUniform1i(getUniformLocation(uniformArray(buf, "gUIDmap", 7, index)), textureUnit);
		camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_COLOR_ATTACHMENT0);
		++textureUnit;
	}*/

	glUniform1i(getUniformLocation("gNumLights"), (GLint)lights.getSize());

	return textureUnit;
}