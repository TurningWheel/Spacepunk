// Shadow.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Shadow.hpp"
#include "Client.hpp"
#include "Renderer.hpp"

Cvar cvar_shadowResolution("render.shadow.resolution", "shadow map resolution", "2048");

const float Shadow::camerainfo_t::fov = 90.f;
const float Shadow::camerainfo_t::clipNear = 1.f;
const float Shadow::camerainfo_t::clipFar = 1000.f;
const Shadow::camerainfo_t Shadow::cameraInfo[Shadow::directions] = {
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, Rotation(PI, PI, 0.f) },					// west
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_X, Rotation(0.f, PI, 0.f) },					// east
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Y, Rotation(-PI / 2.f, -PI / 2.f, 0.f) },	// up
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, Rotation(-PI / 2.f, PI / 2.f, 0.f) },		// down
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Z, Rotation(3.f * PI / 2.f, PI, 0.f) },		// north
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, Rotation(PI / 2.f, PI, 0.f) },			// south
};

Shadow::Shadow() {
}

Shadow::~Shadow() {
	term();
}

void Shadow::init() {
	if (fbo || shadowMap) {
		return;
	}

	// Create the uid cube map
	glGenTextures(1, &uidMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, uidMap);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	const float resolution = cvar_shadowResolution.toFloat();
	for (Uint32 i = 0; i < 6; ++i) {
		glTexImage2D((GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), 0, GL_R32UI, resolution, resolution, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Create the shadow cube map
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (Uint32 i = 0; i < 6; ++i) {
		glTexImage2D((GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), 0, GL_DEPTH_COMPONENT32, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Create the FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (Uint32 i = 0; i < 6; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), shadowMap, 0);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	for (Uint32 i = 0; i < 6; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), uidMap, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);

	// check framebuffer status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to create shadow framebuffer");
	}

	Client* client = mainEngine->getLocalClient(); assert(client);
	Renderer* renderer = client->getRenderer(); assert(renderer);
	Framebuffer* fbo = renderer->getFramebufferResource().dataForString(renderer->getCurrentFramebuffer()); assert(fbo);
	fbo->bindForWriting();
}

void Shadow::term() {
	if (fbo) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (shadowMap) {
		glDeleteTextures(1, &shadowMap);
		shadowMap = 0;
	}
	if (uidMap) {
		glDeleteTextures(1, &uidMap);
		uidMap = 0;
	}
}

void Shadow::bindForWriting(GLenum face) {
	if (!fbo || !shadowMap)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, face, shadowMap, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, uidMap, 0);
	const float resolution = cvar_shadowResolution.toFloat();
	glViewport(0, 0, resolution, resolution);
}

void Shadow::bindForReading(GLenum textureUnit, GLenum attachment) const {
	if (!shadowMap)
		return;
	glActiveTexture(textureUnit);
	if (attachment == GL_DEPTH_ATTACHMENT) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
	} else if (attachment == GL_COLOR_ATTACHMENT0) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, uidMap);
	}
}