// AnimationState.hpp

#pragma once

#include "ArrayList.hpp"
#include "String.hpp"
#include "Animation.hpp"

class Speaker;

class AnimationState {
public:
	AnimationState();
	AnimationState(const Animation::entry_t& entry, const ArrayList<Animation::sound_t>& sounds);
	~AnimationState();

	// sound trigger
	struct sound_t {
		unsigned int frame;
		ArrayList<String> files;

		// save/load this object to a file
		// @param file interface to serialize with
		void serialize(FileInterface * file);
	};

	// advance the animation and play sounds
	// @param speaker: pointer to the speaker used to play animation sounds (if any)
	// @return true if the animation changed, otherwise false
	bool update(Speaker* speaker);

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file);

	// set weight on several bones at once
	// @param bones A list of bones to affect
	// @param weight The weight to set on the bones
	void setWeights(const ArrayList<String>& bones, float weight);

	// set weight rates on several bones at once
	// @param bones A list of bones to affect
	// @param rate The rate to set on the bones
	void setWeightRates(const ArrayList<String>& bones, float rate);

	// clears all weights associated with this animation
	void clearWeights();

	// getters & setters
	const char*					getName() const								{ return name.get(); }
	float						getTicks() const							{ return ticks; }
	float						getTicksRate() const						{ return ticksRate; }
	float						getBegin() const							{ return begin; }
	float						getEnd() const								{ return end; }
	float						getLength() const							{ return length; }
	float						getWeight(const char* bone) const			{ if (const float *weight = weights[bone]) { return *weight; } else { return 0.f; } }
	float						getWeightRate(const char* bone) const		{ if (const float *rate = weightRates[bone]) { return *rate; } else { return 0.f; } }
	bool						isLoop() const								{ return loop; }
	unsigned int				getBeginLastSoundFrame() const				{ return beginLastSoundFrame; }
	unsigned int				getEndLastSoundFrame() const				{ return endLastSoundFrame; }
	const ArrayList<sound_t>&	getSounds() const							{ return sounds; }
	ArrayList<sound_t>&			getSounds()									{ return sounds; }
	bool						isFinished() const							{ return ticks >= (end - begin) && !loop; }

	void	setTicks(float _ticks)											{ ticks = _ticks; updated = true; }
	void	setTicksRate(float _ticksRate)									{ ticksRate = _ticksRate; }
	void	setWeight(const char* bone, float _weight)						{ if (float *weight = weights[bone]) { *weight = _weight; } else { weights.insert(bone, _weight); } updated = true; }
	void	setWeightRate(const char* bone, float _weightRate)				{ if (float *rate = weightRates[bone]) { *rate = _weightRate; } else { weightRates.insert(bone, _weightRate); } }

private:
	String name;				// animation name
	float ticks = 0.f;			// current position in the animation
	float ticksRate = 0.f;		// animation speed
	float begin;				// start frame of the animation
	float end;					// end frame of the animation
	float length;				// length of the animation (end - begin)
	Map<float> weights;			// influences, or "blending" on bones
	Map<float> weightRates;		// rate of change on blending
	bool loop;					// if true, animation loops when ticks > end
	bool updated = false;		// if true, forces the skin to update

	unsigned int beginLastSoundFrame = UINT32_MAX;	// start of range of sound triggers activated last frame
	unsigned int endLastSoundFrame = UINT32_MAX;	// end of range of sound triggers activated last frame
	ArrayList<sound_t> sounds;						// frame sound triggers
};