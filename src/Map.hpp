//! @file Map.hpp

#pragma once

#include "ArrayList.hpp"
#include "Pair.hpp"
#include "File.hpp"

#include <type_traits>

//! A key/value hash map. Provides a fast way to relate one data (the "key") to another (the "value")
template <typename K, typename T>
class Map {
public:
	static const Uint32 maxBucketSize = 1;

	Map() {
		data.resize(numBuckets);
	}
	Map(const Map& src) {
		data.resize(numBuckets);
		copy(src);
	}
	Map(Map&& src) { 
		swap(std::move(src));
	}
	~Map() = default;

	Map& operator=(const Map& src) {
		copy(src);
		return *this;
	}

	Map& operator=(Map&& src) {
		swap(std::move(src));
		return *this;
	}

	ArrayList<OrderedPair<K, T>>&				getHash(Uint32 index) { return data[index]; }
	const ArrayList<OrderedPair<K, T>>&			getHash(Uint32 index) const { return data[index]; }
	Uint32										getNumBuckets() const { return numBuckets; }
	Uint32										getSize() const { return size; }

	//! clears the map of all key/value pairs
	void clear() {
		for (auto& bucket : data) {
			bucket.clear();
		}
		size = 0;
	}

	//! not only clears the map, but also resets its size
	void reset() {
		data.clear();
		size = 0;
		numBuckets = 4;
		data.resize(numBuckets);
	}

	//! inserts a key/value pair into the Map
	//! @param key The key
	//! @param value The value associated with the key
	void insert(const K& key, const T& value) {
		T* oldValue = find(key);
		if (oldValue) {
			*oldValue = value;
			return;
		} else {
			if (size + 1 >= numBuckets * maxBucketSize) {
				rehash(numBuckets * 2);
			}

			auto& list = data[hash(key) & (numBuckets - 1)];
			list.push(OrderedPair<K, T>(key, value));
			++size;
		}
	}

	//! insert a key/value pair into the Map, presuming uniqueness
	//! @param key The key
	//! @param value The value associated with the key
	void insertUnique(const K& key, const T& value) {
		if (size + 1 >= numBuckets * maxBucketSize) {
			rehash(numBuckets * 2);
		}

		auto& list = data[hash(key) & (numBuckets - 1)];
		list.push(OrderedPair<K, T>(key, value));
		++size;
	}

	//! resize and rebuild the hash map
	//! @param newBucketCount Updated number of buckets in the map
	void rehash(Uint32 newBucketCount) {
		ArrayList<OrderedPair<K, T>> list;
		for (auto& it : *this) {
			list.push(it);
		}
		clear();
		numBuckets = newBucketCount;
		data.resize(numBuckets);
		for (auto& it : list) {
			insert(it.a, it.b);
		}
	}

	//! determine if the key with the given name exists
	//! @return true if key/value pair exists, false otherwise
	bool exists(const K& key) const {
		auto& list = data[hash(key) & (numBuckets - 1)];
		for (auto& pair : list) {
			if (pair.a == key) {
				return true;
			}
		}
		return false;
	}

	//! removes a key/value pair from the Map
	//! @param key The key
	//! @return true if the key/value pair was removed, otherwise false
	bool remove(const K& key) {
		auto& list = data[hash(key) & (numBuckets - 1)];
		for (Uint32 c = 0; c < list.getSize(); ++c) {
			auto& pair = list[c];
			if (pair.a == key) {
				list.remove(c);
				--size;
				return true;
			}
		}
		return false;
	}

	//! find the key/value pair with the given name
	//! @param key The name of the pair to find
	//! @return the value associated with the key, or nullptr if it could not be found
	T* find(const K& key) {
		auto& list = data[hash(key) & (numBuckets - 1)];
		for (auto& pair : list) {
			if (pair.a == key) {
				return &pair.b;
			}
		}
		return nullptr;
	}
	const T* find(const K& key) const {
		auto& list = data[hash(key) & (numBuckets - 1)];
		for (auto& pair : list) {
			if (pair.a == key) {
				return &pair.b;
			}
		}
		return nullptr;
	}

	//! replace the contents of this map with those of another
	//! @param src the map to copy
	void copy(const Map& src) {
		clear();
		rehash(src.getNumBuckets());
		for (auto& it : src) {
			insert(it.a, it.b);
		}
	}

	//! swap the contents of this map with those of another
	//! @param src the map to swap with
	void swap(Map&& src) {
		data.swap(std::move(src.data));
		auto tbuckets = numBuckets;
		auto tsize = size;
		numBuckets = src.numBuckets;
		size = src.size;
		src.numBuckets = tbuckets;
		src.size = tsize;
	}

	//! save/load this object to a file
	//! @param file interface to serialize with
	void serialize(FileInterface * file) {
		if (file->isReading()) {
			Uint32 keyCount = 0;
			file->propertyName("data");
			file->beginArray(keyCount);
			for (Uint32 c = 0; c < keyCount; ++c) {
				K key;
				T value;

				file->beginObject();
				file->property("key", key);
				file->property("value", value);
				file->endObject();

				insert(key, value);
			}
			file->endArray();
		} else {
			Uint32 keyCount = 0;
			for (Uint32 c = 0; c < numBuckets; ++c) {
				keyCount += static_cast<Uint32>(data[c].getSize());
			}

			file->propertyName("data");
			file->beginArray(keyCount);
			for (auto& pair : *this) {
				file->beginObject();
				file->property("key", pair.a);
				file->property("value", pair.b);
				file->endObject();
			}
			file->endArray();
		}
	}

	//! find the key/value pair with the given name
	//! @param key The name of the pair to find
	//! @return the value associated with the key
	T* operator[](const K& key) {
		return find(key);
	}
	const T* operator[](const K& key) const {
		return (const T*)(find(key));
	}

	//! Iterator
	class Iterator {
	public:
		Iterator(Map<K, T>& _map, Uint32 _position, Uint32 _bucket) :
			map(_map),
			position(_position),
			bucket(_bucket) {}

		OrderedPair<K, T>& operator*() {
			assert(bucket >= 0 && bucket < map.getNumBuckets());
			assert(position >= 0 && position < map.getHash(bucket).getSize());
			return map.getHash(bucket)[position];
		}
		Iterator& operator++() {
			++position;
			while (bucket < map.getNumBuckets() && position >= map.getHash(bucket).getSize()) {
				++bucket;
				position = 0;
			}
			return *this;
		}
		Iterator& operator--() {
			--position;
			return *this;
		}
		bool operator!=(const Iterator& it) const {
			return position != it.position || bucket != it.bucket;
		}
	private:
		Map<K, T>& map;
		Uint32 position;
		Uint32 bucket;
	};

	//! ConstIterator
	class ConstIterator {
	public:
		ConstIterator(const Map<K, T>& _map, Uint32 _position, Uint32 _bucket) :
			map(_map),
			position(_position),
			bucket(_bucket) {}

		const OrderedPair<K, T>& operator*() const {
			assert(bucket >= 0 && bucket < map.getNumBuckets());
			assert(position >= 0 && position < map.getHash(bucket).getSize());
			return map.getHash(bucket)[position];
		}
		ConstIterator& operator++() {
			++position;
			while (bucket < map.getNumBuckets() && position >= map.getHash(bucket).getSize()) {
				++bucket;
				position = 0;
			}
			return *this;
		}
		ConstIterator& operator--() {
			--position;
			return *this;
		}
		bool operator!=(const ConstIterator& it) const {
			return position != it.position || bucket != it.bucket;
		}
	private:
		const Map<K, T>& map;
		Uint32 position;
		Uint32 bucket;
	};

	//! begin()
	Iterator begin() {
		Uint32 c = 0;
		for (; c < numBuckets; ++c) {
			if (data[c].getSize()) {
				return Iterator(*this, 0, c);
			}
		}
		return Iterator(*this, 0, c);
	}
	const ConstIterator begin() const {
		Uint32 c = 0;
		for (; c < numBuckets; ++c) {
			if (data[c].getSize()) {
				return ConstIterator(*this, 0, c);
			}
		}
		return ConstIterator(*this, 0, c);
	}

	//! end()
	Iterator end() {
		return Iterator(*this, 0, numBuckets);
	}
	const ConstIterator end() const {
		return ConstIterator(*this, 0, numBuckets);
	}

private:
	ArrayList<ArrayList<OrderedPair<K, T>>> data;
	Uint32 numBuckets = 4;
	Uint32 size = 0;

	template <typename Key, std::enable_if_t<std::is_class<Key>::value, unsigned long> = 0>
	unsigned long hash(const Key& key) const {
		return key.hash();
	}
	unsigned long hash(void* key) const {
		return static_cast<unsigned long>(reinterpret_cast<uintptr_t>(key));
	}
	unsigned long hash(Sint32 key) const {
		return static_cast<unsigned long>(key);
	}
	unsigned long hash(Uint32 key) const {
		return static_cast<unsigned long>(key);
	}
	unsigned long hash(Sint64 key) const {
		return static_cast<unsigned long>(key);
	}
	unsigned long hash(Uint64 key) const {
		return static_cast<unsigned long>(key);
	}
	unsigned long hash(bool key) const {
		return key ? 1 : 0;
	}
};
