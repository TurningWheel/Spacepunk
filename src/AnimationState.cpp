// AnimationState.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "AnimationState.hpp"
#include "Speaker.hpp"

AnimationState::AnimationState():
	name("unknown"),
	begin(0.f),
	end(0.f),
	length(0.f),
	loop(false) {}

AnimationState::AnimationState(const Animation::entry_t& entry, const ArrayList<Animation::sound_t>& sounds):
	name(entry.name),
	begin(entry.begin),
	end(entry.end),
	length(end - begin),
	loop(entry.loop)
{
	// copy sound triggers
	for( auto& sound : sounds ) {
		if( sound.frame >= entry.begin && sound.frame < entry.end ) {
			sound_t newSound;
			newSound.frame = sound.frame - begin;
			newSound.files = sound.files;
			this->sounds.push(newSound);
		}
	}
}

AnimationState::~AnimationState() {
}

static Cvar cvar_animRate("anim.rate", "controls rate of model anims", "1.0");

bool AnimationState::update(Speaker* speaker) {
	bool changed = updated;
	updated = false;

	if (weights.getSize() == 0 &&
		weightRates.getSize() == 0) {
		return changed;
	}

	// ticks
	if( length > 1.f ) {
		if (ticksRate && (loop ||
			(ticksRate > 0.f && ticks < length) ||
			(ticksRate < 0.f && ticks > 0.f))) {
			float oldTicks = ticks;
			float animSpeed = ((float)Engine::defaultTickRate / (float)mainEngine->getTicksPerSecond());
			float step = ticksRate * animSpeed;

			ticks += step * cvar_animRate.toFloat();
			if (loop) {
				while (ticks < 0.f) {
					ticks += length;
				}
				while (ticks > length) {
					ticks -= length;
				}
			} else {
				if (ticks < 0.f) {
					ticks = 0.f;
				} else if (ticks > length) {
					ticks = length;
				}
			}

			// play animation sound trigger
			if( speaker ) {
				unsigned int oFrame = (unsigned int) fmod(oldTicks, length);
				unsigned int nFrame = (unsigned int) fmod(ticks, length);
				unsigned int sFrame = step > 0.f ? oFrame : nFrame;
				unsigned int eFrame = step > 0.f ? nFrame : oFrame;
				for( unsigned int c = 0; c < sounds.getSize(); ++c ) {
					const sound_t& sound = sounds[c];
					if( sound.files.getSize() ) {
						if( (sound.frame >= sFrame && sound.frame <= eFrame) &&
							(sound.frame < beginLastSoundFrame || sound.frame > endLastSoundFrame) ) {
							Random& rand = mainEngine->getRandom();
							Uint32 fileIndex = rand.getUint32() % sound.files.getSize();
							const char* file = sound.files[fileIndex].get();
							speaker->playSound(file, false, speaker->getDefaultRange());
							break;
						}
					}
				}
				beginLastSoundFrame = sFrame;
				endLastSoundFrame = eFrame;
			}

			changed = true;
		}
	}

	// blending
	for (auto& pair : weights) {
		float rate = getWeightRate(pair.a.get());
		float& weight = pair.b;
		if ((weight > 0.f && rate < 0.f) || (weight < 1.f && rate > 0.f)) {
			weight += rate;
			if (weight > 1.f) {
				weight = 1.f;
			} else if (weight < 0.f) {
				weight = 0.f;
			}
			changed = true;
		}
	}

	return changed;
}

void AnimationState::setWeights(const ArrayList<String>& bones, float weight) {
	for (auto& bone : bones) {
		setWeight(bone.get(), weight);
	}
}

void AnimationState::setWeightRates(const ArrayList<String>& bones, float rate) {
	for (auto& bone : bones) {
		setWeightRate(bone.get(), rate);
	}
}

void AnimationState::serialize(FileInterface* file) {
	int version = 0;
	file->property("AnimationState::version", version);
	file->property("name", name);
	file->property("ticks", ticks);
	file->property("ticksRate", ticksRate);
	file->property("begin", begin);
	file->property("end", end);
	file->property("length", length);
	file->property("weights", weights);
	file->property("weightRates", weightRates);
	file->property("loop", loop);
	file->property("beginLastSoundFrame", beginLastSoundFrame);
	file->property("endLastSoundFrame", endLastSoundFrame);
	file->property("sounds", sounds);
}

void AnimationState::sound_t::serialize(FileInterface* file) {
	int version = 0;
	file->property("AnimationState::sound_t::version", version);
	file->property("frame", frame);
	file->property("files", files);
}

void AnimationState::clearWeights() {
	weights.clear();
	weightRates.clear();
}