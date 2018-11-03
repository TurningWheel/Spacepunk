// Animation.hpp

#pragma once

#include "Asset.hpp"
#include "String.hpp"
#include "LinkedList.hpp"
#include "File.hpp"

class Animation : public Asset {
public:
	Animation() {}
	Animation(const char* _name);
	virtual ~Animation() {}

	// animation entry
	struct entry_t {
		String name = "unknown";
		unsigned int begin = 0;
		unsigned int end = 1;
		bool loop = false;

		void serialize(FileInterface * file);
	};

	// sound trigger
	struct sound_t {
		unsigned int frame = UINT32_MAX;
		ArrayList<String> files;

		void serialize(FileInterface * file);
	};

	// find the animation with the given name
	// @param name: the name of the animation we are looking for
	// @return a pointer to the animation, or nullptr if it could not be found
	const entry_t* findEntry(const char* name) const;

	// find the sound trigger for the given frame number
	// @param frame: the frame # associated with the sound trigger
	// @return a pointer to the trigger, or nullptr if it could not be found
	const sound_t* findSound(unsigned int frame) const;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface * file);

	// getters & setters
	virtual const type_t		getType() const			{ return ASSET_ANIMATION; }
	const ArrayList<entry_t>&	getEntries() const		{ return entries; }
	const ArrayList<sound_t>&	getSounds() const		{ return sounds; }

private:
	ArrayList<entry_t> entries;
	ArrayList<sound_t> sounds;
};