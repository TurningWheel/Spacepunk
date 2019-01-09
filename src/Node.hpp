// Node.hpp

#pragma once

#include "Main.hpp"
#include "Script.hpp"

template <typename T>
class LinkedList;

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
	void	setData(const T& _data)			{ data = _data; }

	// exposes this node type to a script
	// @param lua The script engine to expose to
	// @param luaTypeName The type name in lua
	static void exposeToScript(sol::state& lua, const char* luaTypeName)
	{
		//First identify the constructors.
		sol::constructors<Node<T>(LinkedList<T>&, Node<T>*, const T&)> constructors;

		//Then do the thing.
		sol::usertype<Node<T>> usertype(constructors,
			"getNext", sol::resolve<Node<T>*()>(&Node<T>::getNext),
			"getPrev", sol::resolve<Node<T>*()>(&Node<T>::getPrev),
			"getList", sol::resolve<LinkedList<T>*()>(&Node<T>::getList),
			"getData", sol::resolve<T&()>(&Node<T>::getData),
			"getNextConst", sol::resolve<const Node<T>*() const>(&Node<T>::getNext),
			"getPrevConst", sol::resolve<const Node<T>*() const>(&Node<T>::getPrev),
			"getListConst", sol::resolve<const LinkedList<T>*() const>(&Node<T>::getList),
			"getDataConst", sol::resolve<const T&() const>(&Node<T>::getData),
			"getSizeOfData", &Node<T>::getSizeOfData,
			"setNext", &Node<T>::setNext,
			"setPrev", &Node<T>::setPrev,
			"setData", &Node<T>::setData
		);

		//Finally register the thing.
		lua.set_usertype(luaTypeName, usertype);
	}

private:
	Node<T>*			next = nullptr;
	Node<T>*			prev = nullptr;
	LinkedList<T>*		list;
	T					data;
};