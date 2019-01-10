// Text.hpp

#pragma once

#include "Asset.hpp"
#include "Main.hpp"
#include "Rect.hpp"

class Text : public Asset {
public:
	Text() {}
	Text(const char* _name);
	virtual ~Text();

	// size of the black text outline
	static const int outlineSize = 1;

	// getters & setters
	virtual const type_t	getType() const		{ return ASSET_TEXT; }
	const GLuint			getTexID() const	{ return texid; }
	const SDL_Surface*		getSurf() const		{ return surf; }
	const unsigned int		getWidth() const	{ return width; }
	const unsigned int		getHeight()	const	{ return height; }

	// draws the text
	// @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	// @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void draw( Rect<int> src, Rect<int> dest ) const;

	// draws the text with the given color
	// @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	// @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void drawColor( Rect<int> src, Rect<int> dest, const glm::vec4& color) const;

private:
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;

	static const GLuint indices[6];
	static const GLfloat positions[8];
	static const GLfloat texcoords[8];
	enum buffer_t {
		VERTEX_BUFFER,
		TEXCOORD_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};
	GLuint vbo[BUFFER_TYPE_LENGTH];
	GLuint vao = 0;

	int width = 0;
	int height = 0;
};