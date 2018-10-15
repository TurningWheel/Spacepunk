// Material.hpp
// Materials combine images, shaders, and other meta-data into a single class

#pragma once

#include "Asset.hpp"
#include "Image.hpp"
#include "Cubemap.hpp"
#include "ShaderProgram.hpp"

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

	// binds all the material textures (should be called after the shader is mounted)
	// @param textureKind: the kind of textures you wish to load
	void bindTextures(texturekind_t textureKind);

	// getters & setters
	virtual const type_t		getType() const				{ return ASSET_MATERIAL; }
	const ShaderProgram&		getShader() const			{ return shader; }
	const LinkedList<Image*>&	getStdTextures() const		{ return stdTextures; }
	const LinkedList<Image*>&	getGlowTextures() const		{ return glowTextures; }
	const bool					isGlowing() const			{ return glowTextures.getSize()>0; }
	ShaderProgram&				getShader()					{ return shader; }
	LinkedList<Image*>&			getStdTextures()			{ return stdTextures; }
	LinkedList<Image*>&			getGlowTextures()			{ return glowTextures; }
	const bool					isTransparent()				{ return transparent; }
	const bool					isShadowing()				{ return shadow; }

	void	setTransparent(bool _transparent)			{ transparent = _transparent; }
	void	setShadowing(bool _shadow)					{ shadow = _shadow; }

private:
	LinkedList<Image*> stdTextures;
	LinkedList<Image*> glowTextures;
	LinkedList<Cubemap*> cubemaps;
	ShaderProgram shader;
	bool transparent = false;
	bool shadow = true;
};