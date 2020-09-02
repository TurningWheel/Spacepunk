//! @file AnimationState.hpp

#pragma once

#include "ArrayList.hpp"
#include "String.hpp"
#include "Animation.hpp"

class Speaker;

//! One AnimationState exists for every animation available to a model for playing.
//! It contains information about which bones an animation affects, the weights on those bones, and more.
class AnimationState {
public:
	AnimationState();
	AnimationState(const Animation::entry_t& entry, const ArrayList<Animation::sound_t>& sounds);
	AnimationState(const AnimationState&) = default;
	AnimationState(AnimationState&&) = default;
	~AnimationState() = default;

	AnimationState& operator=(const AnimationState&) = default;
	AnimationState& operator=(AnimationState&&) = default;

	//! sound trigger
	struct sound_t {
		unsigned int frame;
		ArrayList<String> files;

		//! save/load this object to a file
		//! @param file interface to serialize with
		void serialize(FileInterface * file);
	};

	//! a state pair
	struct state_t {
		state_t() {}
		state_t(float _value, float _rate) :
			value(_value),
			rate(_rate) {}

		float value = 0.f;
		float rate = 0.f;

		//! save/load this object to a file
		//! @param file interface to serialize with
		void serialize(FileInterface * file);
	};

	//! advance the animation and play sounds
	//! @param speaker pointer to the speaker used to play animation sounds (if any)
	//! @return true if the animation changed, otherwise false
	bool update(Speaker* speaker);

	//! save/load this object to a file
	//! @param file interface to serialize with
	void serialize(FileInterface * file);

	//! set weight on several bones at once
	//! @param bones A list of bones to affect
	//! @param weight The weight to set on the bones
	void setWeights(const ArrayList<String>& bones, float weight);

	//! set weight rates on several bones at once
	//! @param bones A list of bones to affect
	//! @param rate The rate to set on the bones
	void setWeightRates(const ArrayList<String>& bones, float rate);

	//! clears all weights associated with this animation
	void clearWeights();

	const char*						getName() const { return name.get(); }
	float							getTicks() const { return ticks; }
	float							getTicksRate() const { return ticksRate; }
	float							getBegin() const { return begin; }
	float							getEnd() const { return end; }
	float							getLength() const { return length; }
	float							getWeight(const char* bone) const { if (const state_t *state = weights[bone]) { return state->value; } else { return 0.f; } }
	float							getWeightRate(const char* bone) const { if (const state_t *state = weights[bone]) { return state->rate; } else { return 0.f; } }
	bool							isLoop() const { return loop; }
	unsigned int					getBeginLastSoundFrame() const { return beginLastSoundFrame; }
	unsigned int					getEndLastSoundFrame() const { return endLastSoundFrame; }
	const ArrayList<sound_t>&		getSounds() const { return sounds; }
	ArrayList<sound_t>&				getSounds() { return sounds; }
	bool							isFinished() const { return ticks >= (end - begin) && !loop; }
	const Map<String, state_t>&		getWeights() const { return weights; }

	void	setTicks(float _ticks) { ticks = _ticks; updated = true; }
	void	setTicksRate(float _ticksRate) { ticksRate = _ticksRate; updated = true; }
	void	setWeight(const char* bone, float _weight) { if (state_t *state = weights[bone]) { state->value = _weight; } else { weights.insert(bone, state_t(_weight, 0.f)); } updated = true; }
	void	setWeightRate(const char* bone, float _weightRate) { if (state_t *state = weights[bone]) { state->rate = _weightRate; } else { weights.insert(bone, state_t(0.f, _weightRate)); } updated = true; }

private:
	String name;					//! animation name
	float ticks = 0.f;				//! current position in the animation
	float ticksRate = 0.f;			//! animation speed
	float begin;					//! start frame of the animation
	float end;						//! end frame of the animation
	float length;					//! length of the animation (end - begin)
	Map<String, state_t> weights;	//! influences, or "blending" on bones
	bool loop;						//! if true, animation loops when ticks > end
	bool updated = false;			//! if true, forces the skin to update

	unsigned int beginLastSoundFrame = UINT32_MAX;	//! start of range of sound triggers activated last frame
	unsigned int endLastSoundFrame = UINT32_MAX;	//! end of range of sound triggers activated last frame
	ArrayList<sound_t> sounds;						//! frame sound triggers
};