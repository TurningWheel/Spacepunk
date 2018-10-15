// Animation.hpp

#pragma once

#include "Asset.hpp"
#include "String.hpp"
#include "LinkedList.hpp"

class Animation : public Asset {
public:
	Animation() {}
	Animation(const char* _name);
	virtual ~Animation() {}

	// animation entry
	struct entry_t {
		String name;
		unsigned int begin;
		unsigned int end;
	};

	// sound trigger
	struct sound_t {
		unsigned int frame;
		ArrayList<String> files;
	};

	// find the animation with the given name
	// @param name: the name of the animation we are looking for
	// @return a pointer to the animation, or nullptr if it could not be found
	const entry_t* findEntry(const char* name) const;

	// find the sound trigger for the given frame number
	// @param frame: the frame # associated with the sound trigger
	// @return a pointer to the trigger, or nullptr if it could not be found
	const sound_t* findSound(unsigned int frame) const;

	// getters & setters
	virtual const type_t		getType() const			{ return ASSET_ANIMATION; }
	const LinkedList<entry_t>&	getEntries() const		{ return entries; }
	const LinkedList<sound_t>&	getSounds() const		{ return sounds; }

private:
	LinkedList<entry_t> entries;
	LinkedList<sound_t> sounds;
};