// LinkedList.hpp

#pragma once

#include "Main.hpp"
#include "Node.hpp"
#include "Script.hpp"

template <typename T>
class LinkedList {
public:
	LinkedList() {}
	~LinkedList() {
		removeAll();
	}

	// parameter functions
	Node<T>* 			getFirst()					{ return first; }
	Node<T>* 			getLast()					{ return last; }

	const Node<T>* 		getFirst() const			{ return (const Node<T>*)(first); }
	const Node<T>* 		getLast() const				{ return (const Node<T>*)(last); }
	size_t				getSize() const				{ return size; }

	void 				setFirst(Node<T> *node)		{ first = node; }
	void 				setLast(Node<T> *node)		{ last = node; }

	// returns the node at the given index
	// @param index the index of the node to be returned
	// @return the Node at the given index, or nullptr if the Node does not exist
	Node<T>* nodeForIndex(const size_t index) {
		if( index>=size ) {
			return nullptr;
		} else if( index>=size/2 ) {
			Node<T>* node=last;
			for( size_t i=size-1; i!=index; node=node->getPrev(), --i );
			return node;
		} else {
			Node<T>* node=first;
			for( size_t i=0; i!=index; node=node->getNext(), ++i );
			return node;
		}
	}
	const Node<T>* nodeForIndex(const size_t index) const {
		if( index>=size ) {
			return nullptr;
		} else if( index>=size/2 ) {
			const Node<T>* node=last;
			for( size_t i=size-1; i!=index; node=node->getPrev(), --i );
			return node;
		} else {
			const Node<T>* node=first;
			for( size_t i=0; i!=index; node=node->getNext(), ++i );
			return node;
		}
	}

	// returns the index of the given node in the list
	// @param node the node for whom you wish to retrieve the index
	// @return the index for the given node, or -1 if the node does not exist in the list
	size_t indexForNode(const Node<T>* node) const {
		if( node->getList() != this )
			return -1;
		if( node==last )
			return size-1;

		Node<T>* tempNode;
		size_t i;

		for( tempNode=first, i=0; tempNode!=nullptr && tempNode!=node; tempNode=tempNode->getNext(), ++i );
		if( tempNode==nullptr ) {
			return -1;
		} else {
			return i;
		}
	}

	// adds a node to the list
	// @param index the position within the list to insert the node
	// @param data the data to be assigned to the node
	// @return the newly created Node
	Node<T>* addNode(const size_t index, const T& data) {
		Node<T>* node = nodeForIndex(index);
		++size;
		return new Node<T>(*this,node,data);
	}

	// adds a node to the beginning of the list
	// @param data the data to be assigned to the node
	// @return the newly created Node
	Node<T>* addNodeFirst(const T& data) {
		++size;
		return new Node<T>(*this,first,data);
	}

	// adds a node to the end of the list
	// @param data the data to be assigned to the node
	// @return the newly created Node
	Node<T>* addNodeLast(const T& data) {
		++size;
		return new Node<T>(*this,nullptr,data);
	}

	// removes a node from the list
	// @param node the node to remove from the list
	void removeNode(Node<T>* node) {
		if( this != node->getList() )
		{
			return;
		}

		if( node == first ) {
			if( node == getLast() ) {
				first = nullptr;
				last = nullptr;
			} else {
				node->getNext()->setPrev(nullptr);
				first = node->getNext();
			}
		} else if( node == last ) {
			node->getPrev()->setNext(nullptr);
			last = node->getPrev();
		} else {
			node->getPrev()->setNext(node->getNext());
			node->getNext()->setPrev(node->getPrev());
		}

		--size;
		delete node;
	}

	// removes a node from the list
	// @param index the index of the node to be removed from the list
	void removeNode(const size_t index) {
		removeNode(nodeForIndex(index));
	}

	// remove all nodes from the list
	void removeAll() {
		Node<T>* node;
		Node<T>* nextnode;

		for( node=first; node!=nullptr; node=nextnode ) {
			nextnode = node->getNext();
			delete node;
		}
		first = nullptr;
		last = nullptr;
		size = 0;
	}

	// copy one list to another (also copies data)
	void copy(const LinkedList<T>& src) {
		for( const Node<T>* node = src.getFirst(); node != nullptr; node = node->getNext() ) {
			Node<T>* newNode = addNodeLast(node->getData());
		}
	}

	// sort the list from least significant elements to most significant elements
	// @return a reference to the sorted list
	LinkedList<T>& sort() {
		size_t len = size;
		if( len<=1 ) {
			return *this;
		}

		size_t i;
		Node<T>* node;
		LinkedList<T> left, right;
		for( i=0, node = first; node!=nullptr && i<len; ++i, node=node->getNext() ) {
			Node<T>* newNode = nullptr;
			if( i%2==0 ) {
				newNode = right.addNodeLast(node->getData());
			} else {
				newNode = left.addNodeLast(node->getData());
			}
		}

		left.sort();
		right.sort();

		return merge(left, right);
	}

	// linear search for the node with the given index
	// @param index the index of the node to be returned
	// @return the Node at the given index, or nullptr if the Node does not exist
	Node<T>* operator[](const size_t index) {
		return nodeForIndex(index);
	}
	const Node<T>* operator[](const size_t index) const {
		return (const Node<T>*)(nodeForIndex(index));
	}

	// Iterator
	class Iterator {
	public:
		Iterator(Node<T>* _position) :
			position(_position) {}

		T& operator*() {
			assert(position != nullptr);
			return position->getData();
		}
		Iterator& operator++() {
			assert(position != nullptr);
			position = position->getNext();
			return *this;
		}
		bool operator!=(const Iterator& it) const {
			return position != it.position;
		}
	private:
		Node<T>* position;
	};

	// ConstIterator
	class ConstIterator {
	public:
		ConstIterator(const Node<T>* _position) :
			position(_position) {}

		const T& operator*() const {
			assert(position != nullptr);
			return position->getData();
		}
		ConstIterator& operator++() {
			assert(position != nullptr);
			position = position->getNext();
			return *this;
		}
		bool operator!=(const ConstIterator& it) const {
			return position != it.position;
		}
	private:
		const Node<T>* position;
	};

	// begin()
	Iterator begin() {
		return Iterator(first);
	}
	const ConstIterator begin() const {
		return ConstIterator(first);
	}

	// end()
	Iterator end() {
		return Iterator(nullptr);
	}
	const ConstIterator end() const {
		return ConstIterator(nullptr);
	}
	
	// exposes this list type to a script
	// @param lua The script engine to expose to
	// @param luaTypeName The type name for the list in lua
	// @param nodeLuaTypeName The type name for the node in lua
	static void exposeToScript(sol::state& lua, const char* luaTypeName, const char* nodeLuaTypeName)
	{
		//First identify the constructors.
		sol::constructors<LinkedList<T>()> constructors;

		//Then do the thing.
		sol::usertype<LinkedList<T>> usertype(constructors,
			"getFirst", sol::resolve<Node<T>*()>(&LinkedList<T>::getFirst),
			"getFirstConst", sol::resolve<const Node<T>*() const>(&LinkedList<T>::getFirst),
			"getLast", sol::resolve<Node<T>*()>(&LinkedList<T>::getLast),
			"getLastConst", sol::resolve<const Node<T>*() const>(&LinkedList<T>::getLast),
			"getSize", &LinkedList<T>::getSize,
			"setFirst", &LinkedList<T>::setFirst,
			"setLast", &LinkedList<T>::setLast,
			"nodeForIndex", sol::resolve<Node<T>*(const size_t)>(&LinkedList<T>::nodeForIndex),
			"nodeForIndexConst", sol::resolve<const Node<T>*(const size_t) const>(&LinkedList<T>::nodeForIndex),
			"indexForNode", &LinkedList<T>::indexForNode,
			"addNode", &LinkedList<T>::addNode,
			"addNodeFirst", &LinkedList<T>::addNodeFirst,
			"addNodeLast", &LinkedList<T>::addNodeLast,
			"removeNode", sol::resolve<void(Node<T>*)>(&LinkedList<T>::removeNode),
			"removeNodeIndex", sol::resolve<void(const size_t)>(&LinkedList<T>::removeNode),
			"removeAll", &LinkedList<T>::removeAll,
			"copy", &LinkedList<T>::copy
		);

		//Finally register the thing.
		lua.set_usertype(luaTypeName, usertype);

		Node<T>::exposeToScript(lua, nodeLuaTypeName);
	}

private:
	Node<T>* first	= nullptr;
	Node<T>* last	= nullptr;
	size_t size = 0;

	LinkedList<T>& merge(LinkedList<T>& left, LinkedList<T>& right) {
		LinkedList<T> result;

		while( left.getFirst() != nullptr && right.getFirst() != nullptr ) {
			if( left.getFirst()->getData() <= right.getFirst()->getData() ) {
				Node<T>* newNode = result.addNodeLast(left.getFirst()->getData());
			} else {
				Node<T>* newNode = result.addNodeLast(right.getFirst()->getData());
			}
		}

		while( left.getFirst() != nullptr ) {
			Node<T>* newNode = result.addNodeLast(left.getFirst()->getData());
		}
		while( right.getFirst() != nullptr ) {
			Node<T>* newNode = result.addNodeLast(right.getFirst()->getData());
		}

		return result;
	}
};
