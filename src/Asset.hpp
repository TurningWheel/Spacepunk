//! @file Asset.hpp

#pragma once

#include "String.hpp"

class FileInterface;

//! Abstract interface that defines an object stored in a Resource hash
class Asset {
public:
	Asset();
	Asset(const char* _name);
	virtual ~Asset();

	//! optional function for streamable types
	//! @return true if the asset was successfully loaded, otherwise false
	virtual bool finalize();

	//! check if a file exists or not on disk
	//! @param name the virtual filename
	//! @return true if the file exists, false if not
	static bool valid(const char* name);

	//! asset type
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
		ASSET_FRAMEBUFFER,
		ASSET_NUM
	};
	static const char* typeStr[ASSET_NUM];

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file);

	virtual const type_t	getType() const { return ASSET_INVALID; }
	virtual const bool		isStreamable() const { return false; }
	const char*				getName() const { return name.get(); }
	const char*				getPath() const { return path.get(); }
	const bool				isLoaded() const { return loaded; }

protected:
	String name;
	String path;
	bool loaded = false;
};