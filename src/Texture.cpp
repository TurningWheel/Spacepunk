// Texture.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Texture.hpp"

const char* Texture::defaultTexture = "images/tile/null.json";
const char* Texture::defaultDiffuse = "images/tile/diffuse/null.png";
const char* Texture::defaultNormal = "images/tile/normal/flat.png";
const char* Texture::defaultEffects = "images/tile/effects/empty.png";

Texture::Texture(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();
	mainEngine->fmsg(Engine::MSG_DEBUG, "loading texture '%s'...", _name);
	if (FileHelper::readObject(path.get(), *this) == false) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to load texture '%s'", _name);
	}
}

Texture::~Texture() {
	textures.clear();
}

void Texture::serialize(FileInterface* file) {
	int version = 0;
	file->property("Texture::version", version);
	file->property("textures", textureStrs);
	if (file->isReading()) {
		for (auto& str : textureStrs) {
			Image* image = mainEngine->getImageResource().dataForString(str.get());
			if (image) {
				textures.push(image);
			}
		}
		if (textures.getSize() < 3) {
			mainEngine->fmsg(Engine::MSG_ERROR, "failed to load texture '%s': 3 textures needed! (diffuse, normal, effects)", name.get());
			loaded = false;
		} else {
			loaded = true;
		}
	}
}