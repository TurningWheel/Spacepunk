//! @file Directory.hpp

#pragma once

#include "Asset.hpp"
#include "LinkedList.hpp"

//! A Directory contains a list of all the files in a computer directory.
class Directory : public Asset {
public:
	Directory() = default;
	Directory(const char* _name);
	Directory(const Directory&) = default;
	Directory(Directory&&) = default;
	virtual ~Directory() = default;

	Directory& operator=(const Directory&) = default;
	Directory& operator=(Directory&&) = default;

	virtual const type_t			getType() const { return ASSET_DIRECTORY; }
	LinkedList<String>&				getList() { return list; }
	const LinkedList<String>&		getList() const { return list; }

private:
	LinkedList<String> list;
};