//! @file Cubemap.hpp

#pragma once

#include "Main.hpp"
#include "Asset.hpp"

//! A Cubemap is a texture with 6 sides.
class Cubemap : public Asset {
public:
	Cubemap() = default;
	Cubemap(const char* _name);
	Cubemap(const Cubemap&) = delete;
	Cubemap(Cubemap&&) = delete;
	virtual ~Cubemap();

	Cubemap& operator=(const Cubemap&) = delete;
	Cubemap& operator=(Cubemap&&) = delete;

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	virtual const type_t	getType() const { return ASSET_CUBEMAP; }
	const GLuint			getTexID() const { return texid; }

private:
	GLuint texid = 0;
	SDL_Surface* surfs[6] = { nullptr };
	String front, back, up, down, right, left;

	bool init();
};