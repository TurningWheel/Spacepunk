// Animation.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Animation.hpp"

Animation::Animation(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();
	loaded = FileHelper::readObject(path.get(), *this);
}

const Animation::entry_t* Animation::findEntry(const char* name) const {
	if (!name) {
		return nullptr;
	}
	for( auto& entry : entries ) {
		if( entry.name == name ) {
			return &entry;
		}
	}
	return nullptr;
}

const Animation::sound_t* Animation::findSound(unsigned int frame) const {
	for( auto& sound : sounds ) {
		if( sound.frame == frame ) {
			return &sound;
		}
	}
	return nullptr;
}

void Animation::serialize(FileInterface* file) {
	int version = 0;
	file->property("Animation::version", version);
	file->property("animations", entries);
	file->property("sounds", sounds);
}

void Animation::entry_t::serialize(FileInterface* file) {
	int version = 0;
	file->property("Animation::entry_t::version", version);
	file->property("name", name);
	file->property("begin", begin);
	file->property("end", end);
	file->property("loop", loop);
}

void Animation::sound_t::serialize(FileInterface* file) {
	int version = 0;
	file->property("Animation::sound_t::version", version);
	file->property("frame", frame);
	file->property("files", files);
}