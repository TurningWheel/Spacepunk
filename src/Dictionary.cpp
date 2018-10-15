// Dictionary.cpp

#include "Dictionary.hpp"

Dictionary::~Dictionary() {
	for( Uint32 c = 0; c < tree.getSize(); ++c ) {
		if( tree[c] ) {
			delete tree[c];
			tree[c] = nullptr;
		}
	}
	tree.clear();
}

void Dictionary::insert(const char* word) {
	if( !word || word[0] == '\0' ) {
		return;
	}

	insert(word, (Uint32)words.getSize());
	String str(word);
	words.push(str);
}

void Dictionary::insert(const char* word, Uint32 _value) {
	if( *word == '\0' ) {
		value = _value; // mark the end of a word
		return;
	}

	size_t len = strlen(word);
	depth = std::max( depth, len );

	// get branch (create new if necessary)
	size_t size = tree.getSize();
	size_t index = (size_t)(*word);
	if( index >= size ) {
		tree.resize(index+1);
		for( size_t c = size; c < tree.getSize(); ++c ) {
			tree[c] = nullptr;
		}
	}
	if( tree[index] == nullptr ) {
		tree[index] = new Dictionary();
	}

	// insert recursively
	tree[index]->insert((const char*)(word+1), _value);
}

bool Dictionary::isLeaf() const {
	if( tree.getSize() > 0 ) {
		return true;
	} else {
		return false;
	}
}

size_t Dictionary::find(const char* word) const {
	if( word == nullptr || *word == '\0' ) {
		return nindex;
	}

	const Dictionary* dict = this;
	while( 1 ) {
		if( *word == '\0' ) {
			return dict->value; // found the end of the word
		} else {
			if( *word >= dict->tree.getSize() ) {
				break;
			} else {
				dict = dict->tree[*word];
				if( dict ) {
					++word;
					continue;
				} else {
					break;
				}
			}
		}
	}

	return nindex; // word not found
}

size_t Dictionary::findOrInsert(const char* word) {
	size_t pos = find(word);
	if (pos == nindex) {
		pos = words.getSize();
		insert(word);
	}
	return pos;
}