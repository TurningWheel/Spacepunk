// Directory.hpp

#pragma once

#include "Asset.hpp"
#include "LinkedList.hpp"

class Directory : public Asset {
public:
	Directory() {}
	Directory(const char* _name);
	virtual ~Directory() {}

	// getters & setters
	virtual const type_t			getType() const		{ return ASSET_DIRECTORY; }
	LinkedList<String>&				getList()			{ return list; }
	const LinkedList<String>&		getList() const		{ return list; }

private:
	LinkedList<String> list;
};