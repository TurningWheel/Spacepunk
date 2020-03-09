// Texture.hpp
// The Texture class is merely a manifest for tile textures (diffuse, normal, and effects maps)
// For mesh textures, see the Material class.

#pragma once

#include "ArrayList.hpp"
#include "Asset.hpp"
#include "Image.hpp"

class Texture : public Asset {
public:
	Texture() {}
	Texture(const char* _name);
	virtual ~Texture();

	// default textures
	static const char* defaultTexture;
	static const char* defaultDiffuse;
	static const char* defaultNormal;
	static const char* defaultEffects;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// getters & setters
	virtual const type_t		getType() const { return ASSET_TEXTURE; }
	const ArrayList<Image*>&	getTextures() const { return textures; }
	ArrayList<Image*>&			getTextures() { return textures; }

private:
	ArrayList<Image*> textures;
	ArrayList<String> textureStrs;
};