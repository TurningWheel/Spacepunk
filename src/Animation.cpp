// Animation.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Animation.hpp"

Animation::Animation(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();

	FILE* fp = NULL;
	mainEngine->fmsg(Engine::MSG_DEBUG,"loading animation '%s'...",_name);
	if( (fp=fopen(path.get(),"rb"))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load animation '%s'",_name);
		return;
	}
	clearerr(fp);

	for( int line=1; !feof(fp); ++line ) {
		char buf[1024] = { 0 };
		if( fgets(buf,1024,fp)==NULL ) {
			break;
		}

		// skip empty lines
		if( buf[0] == '#' || buf[0]=='\n' || buf[0]=='\r' ) {
			continue;
		}

		// null terminate the end of the string
		size_t len = strlen(buf)-1;
		if( buf[len] == '\n' || buf[len] == '\r' ) {
			buf[len] = 0;
			if( len>=1 ) {
				if( buf[len-1] == '\n' || buf[len-1] == '\r' ) {
					buf[len-1] = 0;
				}
			}
		}

		// make an entry
		if( strncmp( buf, "animation ", 10 )==0 ) {
			char* str = (char*)(buf + 10);

			entry_t entry;
			char* token = strtok(str, " ");
			if( token && strcmp(token, "") != 0 ) {
				entry.name = token;
			}
			token = strtok(nullptr, " ");
			if( token ) {
				entry.begin = strtol(token, nullptr, 10);
			}
			token = strtok(nullptr, " ");
			if( token ) {
				entry.end = strtol(token, nullptr, 10);
			}
			entries.addNodeLast(entry);
		}

		// sound effect
		if( strncmp( buf, "sound ", 6 )==0 ) {
			char* str = (char*)(buf + 6);

			sound_t sound;
			char* token = strtok(str, " ");
			if( token && strcmp(token, "") != 0 ) {
				sound.frame = strtol(token, nullptr, 10);
			}
			while( (token = strtok(nullptr, " ")) != nullptr ) {
				String file = token;
				sound.files.push(file);
			}
			sounds.addNodeLast(sound);
		}
	}
	fclose(fp);

	loaded = true;
}

const Animation::entry_t* Animation::findEntry(const char* name) const {
	if( name != nullptr ) {
		for( const Node<entry_t>* node = entries.getFirst(); node != nullptr; node = node->getNext() ) {
			const entry_t& entry = node->getData();
			if( strcmp(entry.name.get(), name) == 0 ) {
				return &entry;
			}
		}
	}
	return nullptr;
}

const Animation::sound_t* Animation::findSound(unsigned int frame) const {
	for( const Node<sound_t>* node = sounds.getFirst(); node != nullptr; node = node->getNext() ) {
		const sound_t& sound = node->getData();
		if( sound.frame == frame ) {
			return &sound;
		}
	}
	return nullptr;
}