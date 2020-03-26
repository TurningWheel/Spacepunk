//! @file Material.hpp

#pragma once

#include "ArrayList.hpp"
#include "Asset.hpp"
#include "Image.hpp"
#include "Cubemap.hpp"
#include "ShaderProgram.hpp"

//! Materials combine Image objects, a ShaderProgram, and other meta-data into a single class
class Material : public Asset {
public:
	Material() {}
	Material(const char* _name);
	virtual ~Material();

	enum texturekind_t {
		STANDARD,
		GLOW,
		TEXTUREKIND_TYPE_LENGTH
	};

	//! binds all the material textures (should be called after the shader is mounted)
	//! @param textureKind the kind of textures you wish to load
	//! @return the next unused texture unit
	unsigned int bindTextures(texturekind_t textureKind);

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	virtual const type_t		getType() const { return ASSET_MATERIAL; }
	const ShaderProgram&		getShader() const { return shader; }
	ShaderProgram&				getShader() { return shader; }
	const bool					isGlowing() const { return glowTextureStrs.getSize() > 0; }
	const bool					isTransparent() { return transparent; }
	const bool					isShadowing() { return shadow; }

	void	setTransparent(bool _transparent) { transparent = _transparent; }
	void	setShadowing(bool _shadow) { shadow = _shadow; }

private:
	ShaderProgram shader;
	ArrayList<String> stdTextureStrs;
	ArrayList<String> glowTextureStrs;
	ArrayList<String> cubemapStrs;
	bool transparent = false;
	bool shadow = true;
};