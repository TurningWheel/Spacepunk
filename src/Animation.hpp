//! @file Animation.hpp

#pragma once

#include "Asset.hpp"
#include "String.hpp"
#include "LinkedList.hpp"
#include "File.hpp"

//! An Animation is actually a data manifest containing all the animations
//! available to a single model for playing.
class Animation : public Asset {
public:
	Animation() = default;
	Animation(const char* _name);
	virtual ~Animation() = default;

	Animation(const Animation&) = delete;
	Animation(Animation&&) = delete;
	Animation& operator=(const Animation&) = delete;
	Animation& operator=(Animation&&) = delete;

	//! animation entry
	struct entry_t {
		String name = "unknown";
		Uint32 begin = 0;
		Uint32 end = 1;
		bool loop = false;

		void serialize(FileInterface * file);
	};

	//! sound trigger
	struct sound_t {
		unsigned int frame = UINT32_MAX;
		ArrayList<String> files;

		void serialize(FileInterface * file);
	};

	//! find the animation with the given name
	//! @param name the name of the animation we are looking for
	//! @return a pointer to the animation, or nullptr if it could not be found
	const entry_t* findEntry(const char* name) const;

	//! find the sound trigger for the given frame number
	//! @param frame the frame # associated with the sound trigger
	//! @return a pointer to the trigger, or nullptr if it could not be found
	const sound_t* findSound(unsigned int frame) const;

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	virtual const Asset::type_t		getType() const { return Asset::ASSET_ANIMATION; }
	const ArrayList<entry_t>&		getEntries() const { return entries; }
	const ArrayList<sound_t>&		getSounds() const { return sounds; }

private:
	ArrayList<entry_t> entries;
	ArrayList<sound_t> sounds;
};