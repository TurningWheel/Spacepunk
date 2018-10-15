// Atlas.hpp

#pragma once

#include "Main.hpp"
#include "LinkedList.hpp"
#include "String.hpp"
#include "Rect.hpp"
#include "ArrayList.hpp"

class Atlas {
public:
	Atlas();
	~Atlas();

	// an image element
	struct pair_t {
		~pair_t() {
			if( surf ) {
				SDL_FreeSurface(surf);
				surf = nullptr;
			}
		}
		SDL_Surface* surf = nullptr;
		String name;
	};

	// invalid index (null image)
	static const GLuint nindex = 0;

	// allocates GL data for the texture atlas
	void init();

	// loads a new image into the atlas ...
	// after all images are loaded, the atlas must be refreshed or rendering will be messed up
	// you cannot load two images with the same name
	// @param _name: the filename of the image to load
	// @return true on success, false on failure
	bool loadImage(const char* _name);

	// uploads all loaded images to the GPU
	void refresh();

	// removes all existing images from the atlas and starts fresh
	void cleanup();

	// find the index of an image
	// @param _name: the name of the image whose index we are looking for
	// @return the index of the image with the given name, or nindex if no such image exists
	const GLuint indexForName(const char* _name) const;

	// getters & setters
	const GLuint			getTexID() const					{ return texid; }
	const String&			getName(GLuint index) const			{ return pairs[index]->name; }
	const SDL_Surface*		getSurf(GLuint index) const			{ return pairs[index]->surf; }
	const unsigned int		getWidth(GLuint index) const		{ return pairs[index]->surf->w; }
	const unsigned int		getHeight(GLuint index)	const		{ return pairs[index]->surf->h; }
	const bool				isValidIndex(GLuint index) const	{ return index<layers; }

private:
	GLuint texid = 0;
	ArrayList<pair_t*> pairs;

	GLuint layers = 0;
	GLint xSize = 0;
	GLint ySize = 0;
};