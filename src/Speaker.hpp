// Speaker.hpp

#pragma once

#include "Main.hpp"
#include "Component.hpp"
#include "String.hpp"
#include "Model.hpp"

class Speaker : public Component {
public:
	Speaker(Entity& _entity, Component* _parent);
	virtual ~Speaker();

	// max sounds per component
	static const int maxSources = 8;

	// speaker model
	static const char* meshStr;
	static const char* materialStr;
	const Mesh::shadervars_t shaderVars;

	// draws the component
	// @param camera the camera through which to draw the component
	// @param light the light by which the component should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	// update the component
	virtual void process() override;

	// plays the given sound
	// @param name the filename of the sound
	// @param loop if true, the sound will loop when played; otherwise, it will not
	// @param range max distance the sound will play over
	// @return the sound source index, or -1 for failure
	int playSound( const char* name, const bool loop, const float range );

	// stops the given sound if it is playing
	// @return true if the sound was successfully stopped, otherwise return false
	bool stopSound( const int index );

	// stops all sounds playing on the speaker
	// @return true if any sound was stopped, otherwise false
	bool stopAllSounds();

	// load the component from a file
	// @param fp the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	// determine if the speaker is playing a sound
	// @return true if the given sound source index is playing
	bool isPlaying(const int index);

	// getters & setters
	virtual type_t		getType() const override		{ return COMPONENT_SPEAKER; }
	const char*			getDefaultSound() const			{ return defaultSound.get(); }
	bool				isDefaultLoop() const			{ return defaultLoop; }
	float				getDefaultRange() const			{ return defaultRange; }

	void		setDefaultSound(const char* name)		{ defaultSound = name; }
	void		setDefaultLoop(const bool b)			{ defaultLoop = b; }
	void		setDefaultRange(const float _range)		{ defaultRange = _range; }

	Speaker& operator=(const Speaker& src) {
		defaultSound = src.defaultSound;
		defaultLoop = src.defaultLoop;
		defaultRange = src.defaultRange;
		stopAllSounds();
		playSound(defaultSound.get(), defaultLoop, defaultRange);
		updateNeeded = true;
		return *this;
	}

private:
	ALuint sources[maxSources];

	String defaultSound;
	bool defaultLoop = false;
	float defaultRange = 256.f;
};