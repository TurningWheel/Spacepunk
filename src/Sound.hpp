//! @file Sound.hpp

#pragma once

#include "Asset.hpp"

//! A Sound is an audio asset loaded from disk. You can play it directly here if you want.
class Sound : public Asset {
public:
	Sound() = default;
	Sound(const char* _name);
	Sound(const Sound&) = delete;
	Sound(Sound&&) = delete;
	virtual ~Sound();

	Sound& operator=(const Sound&) = delete;
	Sound& operator=(Sound&&) = delete;

	//! plays the sound without any listener (2D)
	//! @param loop if true, the sound will loop indefinitely; otherwise, it will only play once
	//! @return the SDL_mixer channel that the sound is playing on, or -1 if there were errors
	int play(const bool loop);

	virtual const type_t	getType() const { return ASSET_SOUND; }
	const ALuint			getBuffer() const { return buffer; }

private:
	ALuint buffer = 0;
	Mix_Chunk* chunk = nullptr;
};