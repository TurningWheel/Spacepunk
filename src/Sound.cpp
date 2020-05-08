// Sound.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Sound.hpp"
#include "Mixer.hpp"

Sound::Sound(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();

	mainEngine->fmsg(Engine::MSG_DEBUG, "loading sound '%s'...", _name);
	if ((chunk = Mix_LoadWAV(path.get())) == NULL) {
		mainEngine->fmsg(Engine::MSG_ERROR, "unable to load sound file '%s': %s", _name, Mix_GetError());
		return;
	}
	alGenBuffers(1, &buffer);
	alBufferData(buffer, AL_FORMAT_MONO16, chunk->abuf, chunk->alen, 44100);

	loaded = true;
}

Sound::~Sound() {
	alDeleteBuffers(1, &buffer);
	Mix_FreeChunk(chunk);
}

int Sound::play(const bool loop) {
	int loops = loop ? -1 : 0;
	int channel = Mix_PlayChannel(-1, chunk, loops);

	// determine volume
	float master = std::min(std::max(0.f, cvar_volumeMaster.toFloat() / 100.f), 1.f);
	float sfx = std::min(std::max(0.f, cvar_volumeSFX.toFloat() / 100.f), 1.f);
	float volume = master * sfx;

	if (channel != -1) {
		Mix_Volume(channel, (int)(volume * 32));
	}
	return channel;
}