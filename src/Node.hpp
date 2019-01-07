// Node.hpp

#pragma once

#include "Main.hpp"

template <typename T>
class LinkedList;

//Forward declare for script engine.
namespace sol
{
	class state;
}

template <typename T>
class Node {
public:
	// insert node with given data anywhere in list
	Node(LinkedList<T>& _list, Node<T>* next, const T& _data):
		list(&_list),
		data(_data)
	{
		if( next==nullptr ) {
			if( list->getLast()==nullptr ) {
				list->setFirst(this);
				list->setLast(this);
			} else {
				setPrev(list->getLast());
				list->getLast()->setNext(this);
				list->setLast(this);
			}
		} else if( next==list->getFirst() ) {
			setNext(list->getFirst());
			list->getFirst()->setPrev(this);
			list->setFirst(this);
		} else {
			Node<T>* prev = next->prev;
			setPrev(prev);
			setNext(next);
			next->setPrev(this);
			prev->setNext(this);
		}
	}

	// will NOT remove node from list
	~Node() {
	}

	// getters & setters
	Node<T>*				getNext()							{ return next; }
	Node<T>*				getPrev()							{ return prev; }
	LinkedList<T>*			getList()							{ return list; }
	T&						getData()							{ return data; }

	const Node<T>*			getNext() const						{ return (const Node<T>*)(next); }
	const Node<T>*			getPrev() const						{ return (const Node<T>*)(prev); }
	const LinkedList<T>*	getList() const						{ return (const LinkedList<T>*)(list); }
	const T&				getData() const						{ return data; }
	const size_t			getSizeOfData() const				{ return (const size_t)sizeof(data); }

	void	setNext(Node<T>* node)		{ next = node; }
	void	setPrev(Node<T>* node)		{ prev = node; }
	void	setData(T& _data)			{ data = _data; }

	// exposes this node type to a script
	// @param lua The script engine to expose to
	// @param name The type name in lua
	static void exposeToScript(sol::state& lua, const char* name);

private:
	Node<T>*			next = nullptr;
	Node<T>*			prev = nullptr;
	LinkedList<T>*		list;
	T					data;
};