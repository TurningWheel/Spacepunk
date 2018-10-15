// Atlas.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Atlas.hpp"

Atlas::Atlas() {
}

Atlas::~Atlas() {
	cleanup();
	if( texid ) {
		glDeleteTextures(1,&texid);
		texid = 0;
	}
}

void Atlas::init() {
	cleanup();

	glGenTextures(1,&texid);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texid);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// load the null image as the base texture
	char name[128];
	snprintf(name,128,"images/system/null.png");
	loadImage(name);
}

void Atlas::cleanup() {
	while( pairs.getSize() > 0 ) {
		pair_t* pair = pairs.pop();
		delete pair;
	}

	xSize = 0;
	ySize = 0;
	layers = 0;
}

bool Atlas::loadImage(const char* _name) {
	String path = mainEngine->buildPath(_name).get();

	SDL_Surface* surf = nullptr;
	if( (surf=IMG_Load(path.get()))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load image '%s'",_name);
		delete surf;
		return false;
	}

	// do not load an image if it already exists in the bank
	for( size_t c = 0; c < pairs.getSize(); ++c ) {
		pair_t* pair = pairs[c];
		String& str = pair->name;
		if( str == _name ) {
			if( surf ) {
				SDL_FreeSurface(surf);
			}
			delete surf;
			return false;
		}
	}

	// translate the original surface to an RGBA surface
	SDL_Surface* newSurf = SDL_CreateRGBSurface(0, surf->w, surf->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_BlitSurface(surf, nullptr, newSurf, nullptr); // blit onto a purely RGBA Surface
	SDL_FreeSurface(surf);
	surf = newSurf;

	// update max size and layer count
	xSize = max(xSize, surf->w);
	ySize = max(ySize, surf->h);
	++layers;

	pair_t* pair = new pair_t();
	pair->name = _name;
	pair->surf = surf;
	pairs.push(pair);

	return true;
}

void Atlas::refresh() {
	glBindTexture(GL_TEXTURE_2D_ARRAY, texid);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 4, GL_RGBA8, xSize, ySize, layers);

	GLuint c = 0;
	Node<pair_t*>* node = nullptr;
	for( size_t c = 0; c < pairs.getSize(); ++c ) {
		pair_t* pair = pairs[c];

		// resize surface
		SDL_Surface* newSurf = SDL_CreateRGBSurface(0, xSize, ySize, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		SDL_BlitScaled(pair->surf, nullptr, newSurf, nullptr);
		SDL_FreeSurface(pair->surf);
		pair->surf = newSurf;

		SDL_LockSurface(pair->surf);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)c, pair->surf->w, pair->surf->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, pair->surf->pixels);
		SDL_UnlockSurface(pair->surf);
	}
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

const GLuint Atlas::indexForName(const char* name) const {
	int c = 0;
	const Node<pair_t*>* node = nullptr;
	for( size_t c = 0; c < pairs.getSize(); ++c ) {
		pair_t* pair = pairs[c];
		if( pair->name == name ) {
			return (GLuint)c;
		}
	}
	return nindex;
}