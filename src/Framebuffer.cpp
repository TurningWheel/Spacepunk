// Framebuffer.cpp

#include "Framebuffer.hpp"
#include "Engine.hpp"

Framebuffer::Framebuffer(const char* _name) : Asset(_name) {
	loaded = true;
}

Framebuffer::~Framebuffer() {
	term();
}

void Framebuffer::init(Uint32 _width, Uint32 _height) {
	if (fbo) {
		return;
	}

	width = _width;
	height = _height;

	// Create the color texture
	glGenTextures(1, &color);
	glBindTexture(GL_TEXTURE_2D, color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create the depth texture
	glGenTextures(1, &depth);
	glBindTexture(GL_TEXTURE_2D, depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create the FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to create framebuffer");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::term() {
	if (fbo) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (color) {
		glDeleteTextures(1, &color);
		color = 0;
	}
	if (depth) {
		glDeleteRenderbuffers(1, &depth);
		depth = 0;
	}
}

void Framebuffer::bindForWriting() {
	if (!fbo)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::bindForReading(GLenum textureUnit, GLenum attachment) const {
	glActiveTexture(textureUnit);
	if (color && attachment == GL_COLOR_ATTACHMENT0) {
		glBindTexture(GL_TEXTURE_2D, color);
	}
	else if (depth && attachment == GL_DEPTH_ATTACHMENT) {
		glBindTexture(GL_TEXTURE_2D, depth);
	}
}