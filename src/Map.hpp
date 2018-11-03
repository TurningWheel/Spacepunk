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
	static const int numBuckets = 128;

	Map() {}
	~Map() {
		clear();
	}

	// getters & setters
	LinkedList<OrderedPair<String, T>>&				getHash(int index)				{ return hash[index]; }
	const LinkedList<OrderedPair<String, T>>&		getHash(int index) const		{ return hash[index]; }

	// clears the map of all key/value pairs
	void clear() {
		for( int c = 0; c < numBuckets; ++c ) {
			while( hash[c].getFirst() ) {
				hash[c].removeNode(hash[c].getFirst());
			}
		}
	}

	// inserts a key/value pair into the Map
	// @param key The key string
	// @param value The value associated with the key
	void insert(const char* key, const T& value) {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) % numBuckets];
		list.addNodeLast(OrderedPair<String, T>(String(key), value));
	}

	// determine if the key with the given name exists
	// @return true if key/value pair exists, false otherwise
	bool exists(const char* key) const {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) % numBuckets];
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
		auto& list = hash[djb2Hash(key) % numBuckets];
		for( auto node = list.getFirst(); node != nullptr; node = node->getNext() ) {
			auto& pair = node->getData();
			if( strcmp(pair.a.get(), key) == 0 ) {
				list.removeNode(node);
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
		auto& list = hash[djb2Hash(key) % numBuckets];
		for( auto& pair : list ) {
			if( strcmp(pair.a.get(), key) == 0 ) {
				return &pair.b;
			}
		}
		return nullptr;
	}
	const T* find(const char* key) const {
		assert(key != nullptr);
		auto& list = hash[djb2Hash(key) % numBuckets];
		for( auto& pair : list ) {
			if( strcmp(pair.a.get(), key) == 0 ) {
				return &pair.b;
			}
		}
		return nullptr;
	}

	// get the size of the map
	// @return the number of entries in the map
	size_t size() const {
		size_t result = 0;
		for( int i = 0; i < numBuckets; ++i ) {
			result += hash[i].getSize();
		}
		return result;
	}

	// replace the contents of this map with those of another
	// @param src The map to copy
	void copy(const Map<T>& src) {
		for (size_t c = 0; c < numBuckets; ++c) {
			hash[c].removeAll();
			hash[c].copy(src.getHash((int)c));
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
			for (Uint32 c = 0; c < Map<String>::numBuckets; ++c) {
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
		Iterator(Map& _map, Node<OrderedPair<String, T>>* _position, int _bucket) :
			map(_map),
			position(_position),
			bucket(_bucket) {}

		OrderedPair<String, T>& operator*() {
			assert(position != nullptr);
			return position->getData();
		}
		Iterator& operator++() {
			assert(position != nullptr);
			position = position->getNext();
			while( position == nullptr ) {
				if( bucket < numBuckets-1 ) {
					++bucket;
					position = map.getHash(bucket).getFirst();
				} else {
					++bucket;
					position = nullptr;
					break;
				}
			}
			return *this;
		}
		bool operator!=(const Iterator& it) const {
			return position != it.position || bucket != it.bucket;
		}
	private:
		Map& map;
		Node<OrderedPair<String, T>>* position;
		int bucket;
	};

	// ConstIterator
	class ConstIterator {
	public:
		ConstIterator(const Map& _map, const Node<OrderedPair<String, T>>* _position, int _bucket) :
			map(_map),
			position(_position),
			bucket(_bucket) {}

		const OrderedPair<String, T>& operator*() const {
			assert(position != nullptr);
			return position->getData();
		}
		ConstIterator& operator++() {
			assert(position != nullptr);
			position = position->getNext();
			while( position == nullptr ) {
				if( bucket < numBuckets-1 ) {
					++bucket;
					position = map.getHash(bucket).getFirst();
				} else {
					++bucket;
					position = nullptr;
					break;
				}
			}
			return *this;
		}
		bool operator!=(const ConstIterator& it) const {
			return position != it.position || bucket != it.bucket;
		}
	private:
		const Map& map;
		const Node<OrderedPair<String, T>>* position;
		int bucket;
	};

	// begin()
	Iterator begin() {
		for (int c = 0; c < numBuckets; ++c) {
			if (hash[c].getFirst()) {
				return Iterator(*this, hash[c].getFirst(), c);
			}
		}
		return Iterator(*this, nullptr, numBuckets);
	}
	const ConstIterator begin() const {
		for (int c = 0; c < numBuckets; ++c) {
			if (hash[c].getFirst()) {
				return ConstIterator(*this, hash[c].getFirst(), c);
			}
		}
		return ConstIterator(*this, nullptr, numBuckets);
	}

	// end()
	Iterator end() {
		return Iterator(*this, nullptr, numBuckets);
	}
	const ConstIterator end() const {
		return ConstIterator(*this, nullptr, numBuckets);
	}

private:
	LinkedList<OrderedPair<String, T>> hash[numBuckets];

	unsigned long djb2Hash(const char* str) const {
		unsigned long hash = 5381;
		int c;

		while((c = *str++)!=0) {
			hash = ((hash << 5) + hash) + c; // hash * 33 + c
		}

		return hash;
	}
};
