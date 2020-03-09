// Sound.hpp

#pragma once

#include "Asset.hpp"

class Sound : public Asset {
public:
	Sound() {}
	Sound(const char* _name);
	virtual ~Sound();

	// plays the sound without any listener (2D)
	// @param loop if true, the sound will loop indefinitely; otherwise, it will only play once
	// @return the SDL_mixer channel that the sound is playing on, or -1 if there were errors
	int play(const bool loop);

	// getters & setters
	virtual const type_t	getType() const { return ASSET_SOUND; }
	const ALuint			getBuffer() const { return buffer; }

private:
	ALuint buffer = 0;
	Mix_Chunk* chunk = nullptr;
};