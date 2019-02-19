// Framebuffer.hpp

#pragma once

#include "Main.hpp"
#include "Asset.hpp"

#pragma once

class Framebuffer : public Asset {
public:
	Framebuffer() {}
	Framebuffer(const char* _name);
	virtual ~Framebuffer();

	// create gl data for framebuffer
	// @param _width the width of the framebuffer
	// @param _height the height of the framebuffer
	void init(Uint32 _width, Uint32 _height);

	// delete gl data for framebuffer
	void term();

	// binds the framebuffer for writing
	void bindForWriting();

	// binds the framebuffer for reading
	// @param textureUnit The texture unit to bind for reading
	// @param attachment The texture we wish to sample (GL_COLOR_ATTACHMENT0 or GL_DEPTH_ATTACHMENT or GL_STENCIL_ATTACHMENT)
	void bindForReading(GLenum textureUnit, GLenum attachment) const;

	// getters & setters
	virtual const type_t	getType() const				{ return ASSET_FRAMEBUFFER; }
	GLuint					getFBO() const				{ return fbo; }
	GLuint					getColor() const			{ return color; }
	GLuint					getDepth() const			{ return depth; }
	GLuint					getStencil() const			{ return stencil; }
	Uint32					getWidth() const			{ return width; }
	Uint32					getHeight() const			{ return height; }
	bool					isInitialized() const		{ return fbo != 0; }

private:
	GLuint fbo = 0;
	GLuint color = 0;
	GLuint depth = 0;
	GLuint stencil = 0;

	Uint32 width = 0;
	Uint32 height = 0;
};