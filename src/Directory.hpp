//! @file Directory.hpp

#pragma once

#include "Asset.hpp"
#include "LinkedList.hpp"

//! A Directory contains a list of all the files in a computer directory.
class Directory : public Asset {
public:
	Directory() {}
	Directory(const char* _name);
	virtual ~Directory() {}

	//! getters & setters
	virtual const type_t			getType() const { return ASSET_DIRECTORY; }
	LinkedList<String>&				getList() { return list; }
	const LinkedList<String>&		getList() const { return list; }

private:
	LinkedList<String> list;
};