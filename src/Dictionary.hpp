//! @file Dictionary.hpp

#pragma once

#include "Main.hpp"
#include "ArrayList.hpp"
#include "String.hpp"

//! A Dictionary is a data structure that enables very fast lookup of a large volume of words, each mapped to a unique integer.
class Dictionary {
public:
	Dictionary() = default;
	Dictionary(const Dictionary&) = default;
	Dictionary(Dictionary&&) = default;
	~Dictionary();

	Dictionary& operator=(const Dictionary&) = default;
	Dictionary& operator=(Dictionary&&) = default;

	//! not a valid index
	static const Uint32 nindex = UINT32_MAX;

	Uint32							getDepth() const { return depth; }
	const ArrayList<String>&		getWords() const { return words; }

	//! insert a word into the tree
	//! @param word the word to insert
	void insert(const char* word);

	//! @return true if this tree node is a leaf
	bool isLeaf() const;

	//! find the given word in the dictionary tree
	//! @param word the word to locate
	//! @return the index of the word in the dictionary, or nindex if it was not found
	Uint32 find(const char* word) const;

	//! find the given word in the dictionary tree or add it if it doesn't exist
	//! @param word the word to locate or insert
	//! @return the index of the word
	Uint32 findOrInsert(const char* word);

private:
	ArrayList<Dictionary*> tree;
	bool found = false;
	Uint32 depth = 0;
	Uint32 value = nindex;
	ArrayList<String> words;

	void insert(const char* word, Uint32 _value);
};