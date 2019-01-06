// Resource.hpp
// Template type must implement Asset

#pragma once

#include "Asset.hpp"
#include "Map.hpp"

template <typename T> class Resource {
public:
	Resource() {}
	~Resource() {
		dumpCache();
	}

	// getters & setters
	Map<T*>&		getCache()			{ return cache; }
	const int		getError() const	{ return error; }

	// number of items in the resource
	// @return the number of cached items in the resource
	size_t size() const {
		return cache.getSize();
	}

	// finds and returns the data with the given name from the cache
	// if the data already exists in the cache, it is returned and error is set to 0
	// if the data does not exist in the cache, the resource creates it anew and error is set to 1
	// if the data failed to be created then error is set to 2
	// otherwise, error is set to 0 and the existing data is returned
	// @param name the name of the data to load
	// @return the data, or nullptr if the data could not be loaded
	T* dataForString( const char* name ) {
		if( name == nullptr || name[0] == '\0' ) {
			return nullptr;
		}

		T** data = cache.find(name);
		if( data ) {
			error = 0;
			return *data;
		} else {
			// data not found, attempt to load it
			T* data = new T(name);
			Asset* base = data; // enforce Asset base class
			if( base->isLoaded() ) {
				error = 1;
				cache.insert(name, data);
				return data;
			} else {
				error = 2;
				delete data;
				return nullptr;
			}
		}
	}

	// completely clears all data elements stored in the cache
	void dumpCache() {
		for( auto& pair : cache ) {
			delete pair.b;
		}
		cache.clear();
	}

private:
	Map<T*> cache;
	int error = 0;
};
