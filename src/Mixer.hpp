//! @file Mixer.hpp

#pragma once

#include "Console.hpp"

class Camera;

//! The Mixer class contains all the state about 3D listeners in the world and the audio engine,
//! and provides methods for playing 2D sounds, among other things.
class Mixer {
public:
	Mixer() = default;
	Mixer(const Mixer&) = delete;
	Mixer(Mixer&&) = delete;
	~Mixer();

	Mixer& operator=(const Mixer&) = delete;
	Mixer& operator=(Mixer&&) = delete;

	//! opens the mixer
	void init();

	//! prints a list of all audio devices available to the mixer
	void listDevices();

	//! set the 3D listener properties for the mixer
	//! @param camera the viewpoint to set the listener properties to
	void setListener(Camera* camera);

	//! play the given sound effect
	//! @param loop if true, the sound will loop indefinitely; otherwise, it will only play once
	//! @return the channel the sound is playing on, or -1 for errors
	int playSound(const char* name, const bool loop);

	//! stop the given sound effect
	//! @param channel the channel the sound is playing on
	//! @return true if the sound was stopped, otherwise false
	bool stopSound(int channel);

	const ALCdevice*	getDevice() const { return device; }
	const ALCcontext*	getContext() const { return context; }
	bool				isInitialized() const { return initialized; }
	Camera*				getListener() { return listener; }

private:
	ALCdevice*	device = nullptr;
	ALCcontext*	context = nullptr;
	Camera* listener = nullptr;

	bool initialized = false;
};

extern Cvar cvar_volumeMaster;
extern Cvar cvar_volumeSFX;
extern Cvar cvar_volumeMusic;