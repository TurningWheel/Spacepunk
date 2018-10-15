// Asset.hpp
// Interface that defines an object stored in a Resource hash

#pragma once

#include "String.hpp"

class FileInterface;

class Asset {
public:
	Asset();
	Asset(const char* _name);
	virtual ~Asset();

	// asset type
	enum type_t {
		ASSET_INVALID,
		ASSET_ANIMATION,
		ASSET_CUBEMAP,
		ASSET_DIRECTORY,
		ASSET_IMAGE,
		ASSET_MATERIAL,
		ASSET_MESH,
		ASSET_SHADER,
		ASSET_SHADERPROGRAM,
		ASSET_SOUND,
		ASSET_TEXT,
		ASSET_TEXTURE,
		ASSET_NUM
	};
	static const char* typeStr[ASSET_NUM];

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file);

	virtual const type_t	getType() const		{ return ASSET_INVALID; }
	const char*				getName() const		{ return name.get(); }
	const char*				getPath() const		{ return path.get(); }
	const bool				isLoaded() const	{ return loaded; }

protected:
	String name;
	String path;
	bool loaded = false;
};