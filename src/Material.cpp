// Material.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Renderer.hpp"
#include "Material.hpp"
#include "Image.hpp"
#include "Cubemap.hpp"

Material::Material(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();
	loaded = FileHelper::readObject(path.get(), *this);
}

Material::~Material() {
}

void Material::serialize(FileInterface* file) {
	int version = 0;
	file->property("Material::version", version);
	file->property("transparent", transparent);
	file->property("shadow", shadow);
	file->property("program", shader);
	file->property("textures", stdTextureStrs);
	file->property("glowTextures", glowTextureStrs);
	file->property("cubemaps", cubemapStrs);
}

unsigned int Material::bindTextures(texturekind_t textureKind) {
	ArrayList<Image*> stdTextures;
	ArrayList<Image*> glowTextures;
	for (auto& path : stdTextureStrs) {
		Image* image = mainEngine->getImageResource().dataForString(path.get());
		if (image) {
			stdTextures.push(image);
		}
	}
	for (auto& path : glowTextureStrs) {
		Image* image = mainEngine->getImageResource().dataForString(path.get());
		if (image) {
			glowTextures.push(image);
		}
	}
	ArrayList<Image*>& images = (textureKind == STANDARD) ? stdTextures : glowTextures;
	unsigned int textureNum = 0;

	// bind normal textures
	if (images.getSize() == 0) {
		Client* client = mainEngine->getLocalClient();
		if (!client) {
			return 0;
		}
		Renderer* renderer = client->getRenderer();
		if (!renderer) {
			return 0;
		} else if (!renderer->isInitialized()) {
			return 0;
		}

		glUniform1i(shader.getUniformLocation("gTexture"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderer->getNullImage()->getTexID());
		++textureNum;
	} else if (images.getSize() == 1) {
		glUniform1i(shader.getUniformLocation("gTexture"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, images[0]->getTexID());
		++textureNum;
	} else if (images.getSize() > 1) {
		for (Uint32 index = 0; index < images.getSize() && textureNum < GL_MAX_TEXTURE_IMAGE_UNITS; ++index, ++textureNum) {
			Image* image = images[index];

			char buf[32] = { 0 };
			snprintf(buf, 32, "gTexture[%d]", (int)index);

			glUniform1i(shader.getUniformLocation(buf), textureNum);
			glActiveTexture(GL_TEXTURE0 + textureNum);
			glBindTexture(GL_TEXTURE_2D, image->getTexID());
		}
	}

	ArrayList<Cubemap*> cubemaps;
	for (auto& path : cubemapStrs) {
		Cubemap* cubemap = mainEngine->getCubemapResource().dataForString(path.get());
		if (cubemap) {
			cubemaps.push(cubemap);
		}
	}

	// bind cubemap textures
	if (cubemaps.getSize() == 1) {
		glUniform1i(shader.getUniformLocation("gCubemap"), textureNum);
		glActiveTexture(GL_TEXTURE0 + std::min(textureNum, (unsigned int)GL_MAX_TEXTURE_IMAGE_UNITS));
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaps[0]->getTexID());
		++textureNum;
	} else if (cubemaps.getSize() > 1) {
		for (Uint32 index = 0; index < cubemaps.getSize() && textureNum < GL_MAX_TEXTURE_IMAGE_UNITS; ++index, ++textureNum) {
			Cubemap* cubemap = cubemaps[index];

			char buf[32] = { 0 };
			snprintf(buf, 32, "gCubemap[%d]", (int)index);

			glUniform1i(shader.getUniformLocation(buf), textureNum);
			glActiveTexture(GL_TEXTURE0 + textureNum);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->getTexID());
		}
	}

	return textureNum;
}