//! @file Node.hpp

#pragma once

#include "Main.hpp"

#include <luajit-2.1/lua.hpp>
#include <LuaBridge/LuaBridge.h>

template <typename T>
class LinkedList;

//! A node lives in a LinkedList
template <typename T>
class Node {
public:
	//! insert node with given data anywhere in list
	Node(LinkedList<T>& _list, Node<T>* next, const T& _data) :
		list(&_list),
		data(_data)
	{
		if (next == nullptr) {
			if (list->getLast() == nullptr) {
				list->setFirst(this);
				list->setLast(this);
			} else {
				setPrev(list->getLast());
				list->getLast()->setNext(this);
				list->setLast(this);
			}
		} else if (next == list->getFirst()) {
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

	Node(const Node&) = default;
	Node(Node&&) = default;

	Node& operator=(const Node&) = default;
	Node& operator=(Node&&) = default;

	//! will NOT remove node from list
	~Node() = default;
	
	friend class LinkedList<T>;

	Node<T>*				getNext() { return next; }
	Node<T>*				getPrev() { return prev; }
	LinkedList<T>*			getList() { return list; }
	T&						getData() { return data; }

	const Node<T>*			getNext() const { return (const Node<T>*)(next); }
	const Node<T>*			getPrev() const { return (const Node<T>*)(prev); }
	const LinkedList<T>*	getList() const { return (const LinkedList<T>*)(list); }
	const T&				getData() const { return data; }
	const Uint32			getSizeOfData() const { return (const Uint32)sizeof(data); }

	void	setNext(Node<T>* node) { next = node; }
	void	setPrev(Node<T>* node) { prev = node; }
	void	setData(T& _data) { data = _data; }

	//! exposes this node type to a script
	//! @param lua The script engine to expose to
	//! @param name The type name in lua
	static void exposeToScript(lua_State* lua, const char* name) {
		typedef Node<T>* (Node<T>::*NodeFn)();
		NodeFn getNext = static_cast<NodeFn>(&Node<T>::getNext);
		NodeFn getPrev = static_cast<NodeFn>(&Node<T>::getPrev);

		typedef LinkedList<T>* (Node<T>::*ListFn)();
		ListFn getList = static_cast<ListFn>(&Node<T>::getList);

		typedef T& (Node<T>::*DataFn)();
		DataFn getData = static_cast<DataFn>(&Node<T>::getData);

		luabridge::getGlobalNamespace(lua)
			.beginClass<Node<T>>(name)
			.addFunction("getNext", getNext)
			.addFunction("getPrev", getPrev)
			.addFunction("getList", getList)
			.addFunction("getData", getData)
			.addFunction("getSizeOfData", &Node<T>::getSizeOfData)
			.endClass()
			;
	}

private:
	Node<T>*			next = nullptr;
	Node<T>*			prev = nullptr;
	LinkedList<T>*		list;
	T					data;
};
