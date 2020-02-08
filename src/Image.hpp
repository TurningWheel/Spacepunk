// Image.hpp

#pragma once

#include "Main.hpp"
#include "Asset.hpp"
#include "Rect.hpp"

class Image : public Asset {
public:
	Image() {}
	Image(const char* _name);
	virtual ~Image();

	virtual bool finalize() override;

	// draws the image
	// @param src the section of the image to be used for drawing, or nullptr for the whole image
	// @param dest the location and size by which the image should be drawn
	void draw(const Rect<int>* src, const Rect<int>& dest) const;

	// draws the image with the given color
	// @param src the section of the image to be used for drawing, or nullptr for the whole image
	// @param dest the location and size by which the image should be drawn
	// @param color a 32-bit color to mix with the image
	void drawColor(const Rect<int>* src, const Rect<int>& dest, const glm::vec4& color) const;

	// create static geometry data for rendering images
	static void createStaticData();

	// delete static geometry data for rendering images
	static void deleteStaticData();

	// getters & setters
	virtual const type_t	getType() const			{ return ASSET_IMAGE; }
	virtual const bool		isStreamable() const	{ return true; }
	const GLuint			getTexID() const		{ return texid; }
	const SDL_Surface*		getSurf() const			{ return surf; }
	const unsigned int		getWidth() const		{ return surf ? surf->w : 0U; }
	const unsigned int		getHeight()	const		{ return surf ? surf->h : 0U; }

private:
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;

	bool clamp = false;
	bool point = false;

	// static geometry data for rendering the image to a quad
	static const GLuint indices[6];
	static const GLfloat positions[8];
	static const GLfloat texcoords[8];
	enum buffer_t {
		VERTEX_BUFFER,
		TEXCOORD_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};
	static GLuint vbo[BUFFER_TYPE_LENGTH];
	static GLuint vao;
};