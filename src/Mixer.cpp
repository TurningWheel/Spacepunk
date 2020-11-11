// Mixer.cpp

#define GLM_FORCE_RADIANS
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Mixer.hpp"

Cvar cvar_volumeMaster("sound.volume.master", "master sound volume (0-100)", "100.0");
Cvar cvar_volumeSFX("sound.volume.sfx", "sound effects volume (0-100)", "100.0");
Cvar cvar_volumeMusic("sound.volume.music", "music volume (0-100)", "100.0");

Mixer::~Mixer() {
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void Mixer::init() {
	mainEngine->fmsg(Engine::MSG_INFO, "initializing OpenAL context...");
	if ((device = alcOpenDevice(NULL)) == NULL) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to open audio device: %d", alGetError());
		return;
	}
	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE) {
		listDevices();
	}
	context = alcCreateContext(device, NULL);
	if (!alcMakeContextCurrent(context)) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to set current audio context");
		return;
	}
	alGetError();

	initialized = true;
}

void Mixer::listDevices() {
	const ALCchar* device = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	const ALCchar* next = device + 1;
	size_t len = 0;

	mainEngine->fmsg(Engine::MSG_INFO, "audio devices found:");
	mainEngine->fmsg(Engine::MSG_INFO, "----------------");
	while (device && *device != '\0' && next && *next != '\0') {
		mainEngine->fmsg(Engine::MSG_INFO, device);
		len = strlen(device);
		device += (len + 1);
		next += (len + 2);
	}
	mainEngine->fmsg(Engine::MSG_INFO, "----------------");
	const ALCchar* defaultDevice = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	mainEngine->fmsg(Engine::MSG_INFO, "selected audio device: %s", defaultDevice);
}

void Mixer::setListener(Camera* camera) {
	listener = camera;
	if (!camera) {
		return;
	}

	float f = 2.f / World::tileSize;

	// determine volume
	float master = std::min(std::max(0.f, cvar_volumeMaster.toFloat() / 100.f), 1.f);
	float sfx = std::min(std::max(0.f, cvar_volumeSFX.toFloat() / 100.f), 1.f);
	float volume = master * sfx;

	// derive orientation
	auto& m = camera->getGlobalMat();
	ALfloat orientation[6] = { m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2] };

	// set listener
	const Vector& pos = camera->getGlobalPos();
	const Vector& vel = camera->getEntity()->getVel();
	alListener3f(AL_POSITION, pos.x*f, -pos.z*f, pos.y*f);
	alListener3f(AL_VELOCITY, vel.x*f, -vel.z*f, vel.y*f);
	alListenerfv(AL_ORIENTATION, orientation);
	alListenerf(AL_GAIN, volume);

	// set global sound volumes as well:
	Mix_Volume(-1, (int)(volume * 32));
}

int Mixer::playSound(const char* name, const bool loop) {
	Sound* sound = mainEngine->getSoundResource().dataForString(StringBuf<64>("%s", 1, name).get());
	if (sound) {
		return sound->play(loop);
	}
	return -1;
}

bool Mixer::stopSound(int channel) {
	return Mix_HaltChannel(channel) == 0;
}