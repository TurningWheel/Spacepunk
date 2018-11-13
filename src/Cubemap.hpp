// Cubemap.hpp

#pragma once

#include "Main.hpp"
#include "Asset.hpp"

class Cubemap : public Asset {
public:
	Cubemap() {}
	Cubemap(const char* _name);
	virtual ~Cubemap();

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// getters & setters
	virtual const type_t	getType() const		{ return ASSET_CUBEMAP; }
	const GLuint			getTexID() const	{ return texid; }

private:
	GLuint texid = 0;
	SDL_Surface* surfs[6] = { nullptr };
	String front, back, up, down, right, left;

	bool init();
};