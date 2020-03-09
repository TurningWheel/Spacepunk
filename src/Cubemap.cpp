// Cubemap.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Cubemap.hpp"

Cubemap::Cubemap(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();
	mainEngine->fmsg(Engine::MSG_DEBUG, "loading cubemap '%s'...", _name);
	if (FileHelper::readObject(path.get(), *this) == false) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to load cubemap '%s'", _name);
	}
}

bool Cubemap::init() {
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	ArrayList<String*> textures = {
		&front, &back, &up, &down, &right, &left
	};

	Uint32 index = 0;
	for (auto& texture : textures) {
		String path = mainEngine->buildPath(texture->get());

		// load image
		if ((surfs[index] = IMG_Load(path.get())) == nullptr) {
			mainEngine->fmsg(Engine::MSG_ERROR, "failed to load cubemap image '%s'", texture->get());
			break;
		}

		// translate the original surface to an RGBA surface
		SDL_Surface* newSurf = SDL_CreateRGBSurface(0, surfs[index]->w, surfs[index]->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		SDL_BlitSurface(surfs[index], nullptr, newSurf, nullptr); // blit onto a purely RGBA Surface
		SDL_FreeSurface(surfs[index]);
		surfs[index] = newSurf;

		// load the new surface as a GL texture
		SDL_LockSurface(surfs[index]);
		if (!index)
		{
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, 4, GL_RGBA8, surfs[index]->w, surfs[index]->h);
		}
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum)index, 0, 0, 0, surfs[index]->w, surfs[index]->h, GL_RGBA, GL_UNSIGNED_BYTE, surfs[index]->pixels);
		SDL_UnlockSurface(surfs[index]);
		++index;
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	if (index < 6) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to load cubemap '%s': some images not found!", name.get());
		return false;
	}

	return true;
}

Cubemap::~Cubemap() {
	for (int c = 0; c < 6; ++c) {
		if (surfs[c]) {
			SDL_FreeSurface(surfs[c]);
			surfs[c] = nullptr;
		}
	}
	if (texid) {
		glDeleteTextures(1, &texid);
		texid = 0;
	}
}

void Cubemap::serialize(FileInterface* file) {
	int version = 0;
	file->property("Cubemap::version", version);
	file->property("front", front);
	file->property("back", back);
	file->property("up", up);
	file->property("down", down);
	file->property("right", right);
	file->property("left", left);
	if (file->isReading()) {
		loaded = init();
	}
}