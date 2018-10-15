// Mixer.hpp

#pragma once

class Camera;

class Mixer {
public:
	Mixer();
	~Mixer();

	// opens the mixer
	void init();

	// prints a list of all audio devices available to the mixer
	void listDevices();

	// set the 3D listener properties for the mixer
	// @param camera: the viewpoint to set the listener properties to
	void setListener(Camera& camera);

	// play the given sound effect
	// @param loop: if true, the sound will loop indefinitely; otherwise, it will only play once
	// @return the Mix_Channel the sound is playing on, or -1 for errors
	int playSound(const char* name, const bool loop);

	// getters & setters
	const ALCdevice*	getDevice() const				{ return device; }
	const ALCcontext*	getContext() const				{ return context; }
	bool				isInitialized() const			{ return initialized; }
	Camera*				getListener()					{ return listener; }
	ALuint				getLowpassFilter() const		{ return filter_lowpass; }

private:
	ALCdevice*	device  = nullptr;
	ALCcontext*	context = nullptr;
	Camera* listener = nullptr;
	ALuint filter_lowpass = 0;

	bool initialized = false;
};