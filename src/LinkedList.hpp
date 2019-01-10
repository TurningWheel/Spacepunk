// LinkedList.hpp

#pragma once

#include "Main.hpp"
#include "Node.hpp"

#include <luajit-2.0/lua.hpp>
#include <LuaBridge/LuaBridge.h>

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
	// @param listName The type name for the list in lua
	// @param nodeName The type name for the node in lua
	static void exposeToScript(lua_State* lua, const char* listName, const char* nodeName) {
		typedef Node<T>* (LinkedList<T>::*NodeFn)();
		NodeFn getFirst = static_cast<NodeFn>(&LinkedList<T>::getFirst);
		NodeFn getLast = static_cast<NodeFn>(&LinkedList<T>::getLast);

		typedef const Node<T>* (LinkedList<T>::*NodeConstFn)() const;
		NodeConstFn getFirstConst = static_cast<NodeConstFn>(&LinkedList<T>::getFirst);
		NodeConstFn getLastConst = static_cast<NodeConstFn>(&LinkedList<T>::getLast);

		typedef Node<T>* (LinkedList<T>::*NodeIndexFn)(const size_t);
		NodeIndexFn nodeForIndex = static_cast<NodeIndexFn>(&LinkedList<T>::nodeForIndex);

		typedef const Node<T>* (LinkedList<T>::*NodeIndexConstFn)(const size_t) const;
		NodeIndexConstFn nodeForIndexConst = static_cast<NodeIndexConstFn>(&LinkedList<T>::nodeForIndex);

		typedef void (LinkedList<T>::*NodeRemoveFn)(Node<T>*);
		NodeRemoveFn removeNode = static_cast<NodeRemoveFn>(&LinkedList<T>::removeNode);

		typedef void (LinkedList<T>::*NodeRemoveIndexFn)(const size_t);
		NodeRemoveIndexFn removeNodeIndex = static_cast<NodeRemoveIndexFn>(&LinkedList<T>::removeNode);

		luabridge::getGlobalNamespace(lua)
			.beginClass<LinkedList<T>>(listName)
			.addConstructor<void (*)()>()
			.addFunction("getFirst", getFirst)
			.addFunction("getFirstConst", getFirstConst)
			.addFunction("getLast", getLast)
			.addFunction("getLastConst", getLastConst)
			.addFunction("getSize", &LinkedList<T>::getSize)
			.addFunction("setFirst", &LinkedList<T>::setFirst)
			.addFunction("setLast", &LinkedList<T>::setLast)
			.addFunction("nodeForIndex", nodeForIndex)
			.addFunction("nodeForIndexConst", nodeForIndexConst)
			.addFunction("indexForNode", &LinkedList<T>::indexForNode)
			.addFunction("addNode", &LinkedList<T>::addNode)
			.addFunction("addNodeFirst", &LinkedList<T>::addNodeFirst)
			.addFunction("addNodeLast", &LinkedList<T>::addNodeLast)
			.addFunction("removeNode", removeNode)
			.addFunction("removeNodeIndex", removeNodeIndex)
			.addFunction("removeAll", &LinkedList<T>::removeAll)
			.addFunction("copy", &LinkedList<T>::copy)
			.endClass()
		;

		Node<T>::exposeToScript(lua, nodeName);
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
