// Resource.hpp
// Template type must implement Asset

#pragma once

#include <future>
#include <unordered_map>
#include <string>

#include "Asset.hpp"
#include "Map.hpp"

enum resource_error_t {
	ERROR_NONE,				// no error
	ERROR_NOTCACHED,		// resource had to be cached
	ERROR_CACHEFAILED,		// resource cache failed
	ERROR_CACHEINPROGRESS	// resource cache is in progress
};

template <typename T, bool stream = false> class Resource {
public:
	Resource() {}
	~Resource() {
		dumpCache();
	}

	// getters & setters
	Map<String, T*>&			getCache() { return cache; }
	const resource_error_t		getError() const { return error; }

	// number of items in the resource
	// @return the number of cached items in the resource
	Uint32 size() const {
		return cache.getSize();
	}

	// finds and returns the data with the given name from the cache
	// if the data already exists in the cache, it is returned and error is set to ERROR_NONE
	// if the data does not exist in the cache, the resource creates it anew and error is set to ERROR_NOTCACHED
	// if the data failed to be created then error is set to ERROR_CACHEFAILED
	// if the data is currently being streamed then error is set to ERROR_CACHEINPROGRESS
	// @param name the name of the data to load
	// @return the data, or nullptr if the data could not be loaded
	T* dataForString(const char* name) {
		if (name == nullptr || name[0] == '\0') {
			error = resource_error_t::ERROR_CACHEFAILED;
			return nullptr;
		}

		T** data = cache.find(name);
		if (data) {
			error = resource_error_t::ERROR_NONE;
			return *data;
		} else {
			// data not found, attempt to load it
			T* data = nullptr;
			if (stream) {
				if (cache.getSize()) {
					auto it = jobs.find(name);
					if (it == jobs.end()) {
						jobs.emplace(name, std::async(std::launch::async, &Resource::load, this, name));
					}
					error = resource_error_t::ERROR_CACHEINPROGRESS;
					return nullptr;
				} else {
					data = load(name);
					data->finalize();
				}
			} else {
				data = load(name);
				data->finalize();
			}
			Asset* base = data; // enforce Asset base class
			if (base->isLoaded()) {
				error = resource_error_t::ERROR_NOTCACHED;
				cache.insertUnique(name, data);
				return data;
			} else {
				error = resource_error_t::ERROR_CACHEFAILED;
				delete data;
				return nullptr;
			}
		}
	}

	// finishes jobs
	void update() {
		std::vector<std::string> keys;
		for (auto& pair : jobs) {
			auto& name = pair.first;
			auto& job = pair.second;
			auto status = job.wait_for(std::chrono::milliseconds(0));
			if (status == std::future_status::ready) {
				T* data = job.get();
				data->finalize();
				Asset* base = data; // enforce Asset base class
				if (base->isLoaded()) {
					cache.insertUnique(name.c_str(), data);
					keys.push_back(name);
				}
			}
		}
		for (auto& key : keys) {
			jobs.erase(key);
		}
	}

	// completely clears all data elements stored in the cache
	void dumpCache() {
		for (auto& job : jobs) {
			job.second.wait();
		}
		for (auto& pair : cache) {
			delete pair.b;
		}
		cache.clear();
	}

	// delete some specific data from the cache
	void deleteData(const char* name) {
		T** data = cache.find(name);
		if (data) {
			delete *data;
			cache.remove(name);
		}
	}

	// calculate the size of this resource cache
	Uint32 getSizeInBytes() const {
		// need something better than sizeof
		return (Uint32)(cache.getSize() * sizeof(T));
	}

private:
	Map<String, T*> cache;
	std::unordered_map<std::string, std::future<T*>> jobs;
	resource_error_t error = resource_error_t::ERROR_NONE;

	T* load(const char* name) {
		return new T(name);
	}
};
