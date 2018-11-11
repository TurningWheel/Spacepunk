// Map.hpp
// Key/value hash map

#pragma once

#include <type_traits>
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Pair.hpp"
#include "String.hpp"
#include "File.hpp"

template <typename T>
class Map {
public:
	static const size_t maxBucketSize = 1;

	Map() {
		hash.resize(numBuckets);
	}
	~Map() {
		clear();
	}

	// getters & setters
	ArrayList<OrderedPair<String, T>>&				getHash(size_t index)			{ return hash[index]; }
	const ArrayList<OrderedPair<String, T>>&		getHash(size_t index) const		{ return hash[index]; }
	size_t											getNumBuckets() const			{ return numBuckets; }
	size_t											getSize() const					{ return size; }

	// clears the map of all key/value pairs
	void clear() {
		for (auto& bucket : hash) {
			bucket.clear();
		}
		size = 0;
	}

	// not only clears the map, but also resets its size
	void reset() {
		hash.clear();
		size = 0;
		numBuckets = 4;
		hash.resize(numBuckets);
	}

	// inserts a key/value pair into the Map
	// @param key The key string
	// @param value The value associated with the key
	void insert(const char* key, const T& value) {
		assert(key != nullptr);

		if (size + 1 >= numBuckets * maxBucketSize) {
			rehash(numBuckets * 2);
		}

		auto& list = hash[djb2Hash(key) & (numBuckets - 1)];
		list.push(OrderedPair<String, T>(String(key), value));
		++size;
	}

	// resize and rebuild the hash map
	// @param newBucketCount Updated number of buckets in the map
	void rehash(size_t newBucketCount) {
		ArrayList<OrderedPair<String, T>> list;
		for (auto& it : *this) {
			list.push(it);
		}
		clear();
		numBuckets = newBucketCount;
		hash.resize(numBuckets);
		for (auto& it : list) {
			insert(it.a.get(), it.b);
		}
	}

	// determine if the key with the given name exists
	// @return true if key/value pair exists, false otherwise
	bool exists(const char* key) const {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) & (numBuckets - 1)];
		for( auto& pair : list ) {
			if( strcmp(pair.a.get(), key) == 0 ) {
				return true;
			}
		}
		return false;
	}

	// removes a key/value pair from the Map
	// @param key The key string
	// @return true if the key/value pair was removed, otherwise false
	bool remove(const char* key) {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) & (numBuckets - 1)];
		for( size_t c = 0; c < list.getSize(); ++c ) {
			auto& pair = list[c];
			if( strcmp(pair.a.get(), key) == 0 ) {
				list.remove(c);
				--size;
				return true;
			}
		}
		return false;
	}

	// find the key/value pair with the given name
	// @param key The name of the pair to find
	// @return the value associated with the key, or nullptr if it could not be found
	T* find(const char* key) {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) & (numBuckets - 1)];
		for( auto& pair : list ) {
			if( strcmp(pair.a.get(), key) == 0 ) {
				return &pair.b;
			}
		}
		return nullptr;
	}
	const T* find(const char* key) const {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) & (numBuckets - 1)];
		for( auto& pair : list ) {
			if( strcmp(pair.a.get(), key) == 0 ) {
				return &pair.b;
			}
		}
		return nullptr;
	}

	// replace the contents of this map with those of another
	// @param src The map to copy
	void copy(const Map<T>& src) {
		clear();
		rehash(src.getNumBuckets());
		for (auto& it : src) {
			insert(it.a.get(), it.b);
		}
	}

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file) {
		if (file->isReading()) {
			Uint32 keyCount = 0;
			file->propertyName("data");
			file->beginArray(keyCount);
			for( Uint32 c = 0; c < keyCount; ++c ) {
				String key;
				T value;

				file->beginObject();
				file->property("key", key);
				file->property("value", value);
				file->endObject();

				insert(key.get(), value);
			}
			file->endArray();
		} else {
			Uint32 keyCount = 0;
			for (Uint32 c = 0; c < numBuckets; ++c) {
				keyCount += static_cast<Uint32>(hash[c].getSize());
			}

			file->propertyName("data");
			file->beginArray(keyCount);
			for( auto& pair : *this ) {
				file->beginObject();
				file->property("key", pair.a);
				file->property("value", pair.b);
				file->endObject();
			}
			file->endArray();
		}
	}

	// find the key/value pair with the given name
	// @param key The name of the pair to find
	// @return the value associated with the key
	T* operator[](const char* str) {
		return find(str);
	}
	const T* operator[](const char* str) const {
		return (const T*)(find(str));
	}

	// Iterator
	class Iterator {
	public:
		Iterator(Map& _map, size_t _position, size_t _bucket) :
			map(_map),
			position(_position),
			bucket(_bucket) {}

		OrderedPair<String, T>& operator*() {
			assert(bucket >= 0 && bucket < map.getNumBuckets());
			assert(position >= 0 && position < map.getHash(bucket).getSize());
			return map.getHash(bucket)[position];
		}
		Iterator& operator++() {
			++position;
			while( bucket < map.getNumBuckets() && position >= map.getHash(bucket).getSize() ) {
				++bucket;
				position = 0;
			}
			return *this;
		}
		bool operator!=(const Iterator& it) const {
			return position != it.position || bucket != it.bucket;
		}
	private:
		Map& map;
		size_t position;
		size_t bucket;
	};

	// ConstIterator
	class ConstIterator {
	public:
		ConstIterator(const Map& _map, size_t _position, size_t _bucket) :
			map(_map),
			position(_position),
			bucket(_bucket) {}

		const OrderedPair<String, T>& operator*() const {
			assert(bucket >= 0 && bucket < map.getNumBuckets());
			assert(position >= 0 && position < map.getHash(bucket).getSize());
			return map.getHash(bucket)[position];
		}
		ConstIterator& operator++() {
			++position;
			while( bucket < map.getNumBuckets() && position >= map.getHash(bucket).getSize() ) {
				++bucket;
				position = 0;
			}
			return *this;
		}
		bool operator!=(const ConstIterator& it) const {
			return position != it.position || bucket != it.bucket;
		}
	private:
		const Map& map;
		size_t position;
		size_t bucket;
	};

	// begin()
	Iterator begin() {
		size_t c = 0;
		for (; c < numBuckets; ++c) {
			if (hash[c].getSize()) {
				return Iterator(*this, 0, c);
			}
		}
		return Iterator(*this, 0, c);
	}
	const ConstIterator begin() const {
		size_t c = 0;
		for (; c < numBuckets; ++c) {
			if (hash[c].getSize()) {
				return ConstIterator(*this, 0, c);
			}
		}
		return ConstIterator(*this, 0, c);
	}

	// end()
	Iterator end() {
		return Iterator(*this, 0, numBuckets);
	}
	const ConstIterator end() const {
		return ConstIterator(*this, 0, numBuckets);
	}

private:
	ArrayList<ArrayList<OrderedPair<String, T>>> hash;
	size_t numBuckets = 4;
	size_t size = 0;

	unsigned long djb2Hash(const char* str) const {
		unsigned long hash = 5381;
		int c;

		while((c = *str++)!=0) {
			hash = ((hash << 5) + hash) + c; // hash * 33 + c
		}

		return hash;
	}
};
