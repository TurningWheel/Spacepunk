// Directory.cpp

#include <dirent.h>

#include "Main.hpp"
#include "Engine.hpp"
#include "Directory.hpp"

Directory::Directory(const char* _name) : Asset(_name) {
	name = _name;

	DIR* dir;
	struct dirent* ent;
	if( (dir=opendir(_name)) == NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to open directory '%s'",_name);
		return;
	}
	while( (ent=readdir(dir)) != NULL ) {
		String entry(ent->d_name);
		if( entry=="." || entry==".." )
			continue;

		unsigned int c = 0;
		Node<String>* node = nullptr;
		for( c = 0, node = list.getFirst(); node != nullptr; node = node->getNext(), ++c ) {
			if( node->getData() > entry.get() ) {
				break;
			}
		}
		list.addNode(c, entry);
	}
	closedir( dir );
	loaded = true;
}