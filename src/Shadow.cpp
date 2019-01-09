// Shadow.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Shadow.hpp"

const float Shadow::camerainfo_t::fov = 90.f;
const float Shadow::camerainfo_t::clipNear = 1.f;
const float Shadow::camerainfo_t::clipFar = 1000.f;
const Rect<Sint32> Shadow::camerainfo_t::win = Rect<Sint32>(0, 0, Shadow::resolution, Shadow::resolution);
const Shadow::camerainfo_t Shadow::cameraInfo[Shadow::directions] = {
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, Angle(PI, PI, 0.f) },					// west
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_X, Angle(0.f, PI, 0.f) },				// east
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Y, Angle(-PI / 2.f, -PI / 2.f, 0.f) },	// up
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, Angle(-PI / 2.f, PI / 2.f, 0.f) },	// down
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Z, Angle(3.f * PI / 2.f, PI, 0.f) },		// north
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, Angle(PI / 2.f, PI, 0.f) },			// south
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

	// Create the cube map
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (size_t i = 0; i < 6; ++i) {
		glTexImage2D((GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), 0, GL_DEPTH_COMPONENT32, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Create the FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (size_t i = 0; i < 6; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), shadowMap, 0);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	// Disable writes to the color buffer
	glDrawBuffer(GL_NONE);

	// Disable reads from the color buffer
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
}

void Shadow::bindForWriting(GLenum face) {
	if (!fbo || !shadowMap)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, face, shadowMap, 0);
}

void Shadow::bindForReading(GLenum textureUnit) const {
	if (!shadowMap)
		return;
	glActiveTexture(textureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
}