// Framebuffer.cpp

#include "Framebuffer.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Renderer.hpp"

Cvar cvar_antialias("render.multisample", "the number of samples to use for antialiasing (0 = off)", "4");

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

	GLint maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	samples = std::min(std::max(0, cvar_antialias.toInt()), (int)maxSamples);

	glActiveTexture(GL_TEXTURE0);

	// Create the FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create the color textures
	glGenTextures(ColorBuffer::MAX, color);
	if (samples) {
		for (int c = 0; c < ColorBuffer::MAX; ++c) {
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color[c]);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA32F, width, height, false);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_TEXTURE_2D_MULTISAMPLE, color[c], 0);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		}
	} else {
		for (int c = 0; c < ColorBuffer::MAX; ++c) {
			glBindTexture(GL_TEXTURE_2D, color[c]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_TEXTURE_2D, color[c], 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	// Create the depth texture
	glGenTextures(1, &depth);
	if (samples) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depth);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_DEPTH32F_STENCIL8, width, height, false);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depth, 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	} else {
		glBindTexture(GL_TEXTURE_2D, depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// finalize fbo
	static const GLenum attachments[ColorBuffer::MAX] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(ColorBuffer::MAX, attachments);
	glReadBuffer(GL_NONE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// check fbo status
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
	for (int c = 0; c < ColorBuffer::MAX; ++c) {
		if (color[c]) {
			glDeleteTextures(1, &color[c]);
			color[c] = 0;
		}
	}
	if (depth) {
		glDeleteTextures(1, &depth);
		depth = 0;
	}
	if (stencil) {
		glDeleteTextures(1, &stencil);
		stencil = 0;
	}
}

void Framebuffer::bindForWriting() {
	if (!fbo)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLenum type = samples ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	for (int c = 0; c < ColorBuffer::MAX; ++c) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, type, color[c], 0);
	}
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, type, depth, 0);
	glViewport(0, 0, width, height);

	// remember which fbo is bound for writing
	Client* client = mainEngine->getLocalClient(); assert(client);
	Renderer* renderer = client->getRenderer(); assert(renderer);
	renderer->setCurrentFramebuffer(name.get());
}

void Framebuffer::bindForReading(GLenum textureUnit, GLenum attachment) const {
	glActiveTexture(textureUnit);
	GLenum type = samples ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	for (int c = 0; c < ColorBuffer::MAX; ++c) {
		if (color[c] && attachment == GL_COLOR_ATTACHMENT0 + c) {
			glBindTexture(type, color[c]);
			return;
		}
	}
	if (depth && attachment == GL_DEPTH_ATTACHMENT) {
		glBindTexture(type, depth);
		return;
	}
}

void Framebuffer::clear() {
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glClearDepth(0.f);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}