//! @file Text.hpp

#pragma once

#include "Asset.hpp"
#include "Main.hpp"
#include "Rect.hpp"
#include "String.hpp"

//! Contains some text that was rendered to a texture with a ttf font.
class Text : public Asset {
public:
	Text() = default;
	Text(const char* _name);
	Text(const Text&) = delete;
	Text(Text&&) = delete;
	virtual ~Text();

	Text& operator=(const Text&) = delete;
	Text& operator=(Text&&) = delete;

	//! size of the black text outline
	static const int outlineSize = 1;

	//! special char marks font to be used
	static const char fontBreak = 8;

	//! get a Text object from the engine
	//! @param str The Text's string
	//! @param font the Text's font
	//! @return the Text or nullptr if it could not be retrieved
	static Text* get(const char* str, const char* font);

	virtual type_t	        getType() const { return ASSET_TEXT; }
	GLuint			        getTexID() const { return texid; }
	const SDL_Surface*		getSurf() const { return surf; }
	unsigned int		    getWidth() const { return width; }
	unsigned int		    getHeight()	const { return height; }

	//! renders the text using its pre-specified parameters. this should usually not be called by the user.
	void render();

	//! draws the text
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void draw(Rect<int> src, Rect<int> dest);

	//! draws the text with the given color
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void drawColor(Rect<int> src, Rect<int> dest, const glm::vec4& color);

	//! create static geometry data for rendering images
	static void createStaticData();

	//! delete static geometry data for rendering images
	static void deleteStaticData();

private:
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;

	//! static geometry data for rendering the image to a quad
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

	int width = 0;
	int height = 0;
	bool rendered = false;
};
