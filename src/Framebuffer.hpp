//! @file Framebuffer.hpp

#pragma once

#include "Main.hpp"
#include "Asset.hpp"
#include "Console.hpp"

//! A Framebuffer is a unique type of object belonging to the Renderer which basically represents a "screen" texture that can be rendered to.
class Framebuffer : public Asset {
public:
	Framebuffer() = default;
	Framebuffer(const char* _name);
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer(Framebuffer&&) = delete;
	virtual ~Framebuffer();

	Framebuffer& operator=(const Framebuffer&) = delete;
	Framebuffer& operator=(Framebuffer&&) = delete;

	//! color buffers
	enum ColorBuffer {
		COLOR,
		BLOOM,
		MAX
	};

	//! create gl data for framebuffer
	//! @param _width the width of the framebuffer
	//! @param _height the height of the framebuffer
	void init(Uint32 _width, Uint32 _height);

	//! delete gl data for framebuffer
	void term();

	//! unbind framebuffer from writing
	static void unbind();

	//! binds the framebuffer for writing
	void bindForWriting();

	//! binds the framebuffer for reading
	//! @param textureUnit The texture unit to bind for reading
	//! @param attachment The texture we wish to sample (GL_COLOR_ATTACHMENT0 or GL_DEPTH_ATTACHMENT or GL_STENCIL_ATTACHMENT)
	void bindForReading(GLenum textureUnit, GLenum attachment) const;

	//! clear buffers
	void clear();

	virtual const type_t	getType() const { return ASSET_FRAMEBUFFER; }
	GLuint					getFBO() const { return fbo; }
	GLuint					getColor(int c) const { return color[c]; }
	GLuint					getDepth() const { return depth; }
	GLuint					getStencil() const { return stencil; }
	Uint32					getWidth() const { return width; }
	Uint32					getHeight() const { return height; }
	bool					isInitialized() const { return fbo != 0; }

private:
	GLuint fbo = 0;
	GLuint color[ColorBuffer::MAX];
	GLuint depth = 0;
	GLuint stencil = 0;

	Uint32 width = 0;
	Uint32 height = 0;
	Uint32 samples = 0;
};

extern Cvar cvar_antialias;