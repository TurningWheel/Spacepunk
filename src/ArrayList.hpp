//! @file ArrayList.hpp

#pragma once

#include "Main.hpp"

#include <luajit-2.1/lua.hpp>
#include <LuaBridge/LuaBridge.h>

//! templated ArrayList (similar to std::vector)
//! adding or removing elements can unsort the list.
//! @param T generic type that the list will contain
template <typename T>
class ArrayList {
public:
	ArrayList<T>() = default;

	ArrayList(const ArrayList& src) {
		copy(src);
	}

	ArrayList(ArrayList&& src) {
		swap(std::move(src));
	}

	ArrayList(const std::initializer_list<T>& src) {
		copy(src);
	}

	virtual ~ArrayList() {
		if (arr) {
			delete[] arr;
			arr = nullptr;
		}
	}

	const T*		getArray() const { return arr; }
	T*				getArray() { return arr; }
	Uint32			getSize() const { return size; }
	Uint32			getMaxSize() const { return maxSize; }

	//! @return true if list is empty
	bool empty() const {
		return (size == 0);
	}

	//! Iterator
	class Iterator {
	public:
		Iterator(ArrayList<T>& _arr, Uint32 _pos) :
			arr(_arr),
			pos(_pos) {}

		T& operator*() {
			assert(pos >= 0 && pos < arr.getSize());
			return arr[pos];
		}
		Iterator& operator++() {
			++pos;
			return *this;
		}
		bool operator!=(const Iterator& it) const {
			return pos != it.pos;
		}
	private:
		ArrayList<T>& arr;
		Uint32 pos;
	};

	//! ConstIterator
	class ConstIterator {
	public:
		ConstIterator(const ArrayList<T>& _arr, Uint32 _pos) :
			arr(_arr),
			pos(_pos) {}

		const T& operator*() const {
			assert(pos >= 0 && pos < arr.getSize());
			return arr[pos];
		}
		ConstIterator& operator++() {
			++pos;
			return *this;
		}
		bool operator!=(const ConstIterator& it) const {
			return pos != it.pos;
		}
	private:
		const ArrayList<T>& arr;
		Uint32 pos;
	};

	//! begin()
	Iterator begin() {
		return Iterator(*this, 0);
	}
	const ConstIterator begin() const {
		return ConstIterator(*this, 0);
	}

	//! end()
	Iterator end() {
		return Iterator(*this, size);
	}
	const ConstIterator end() const {
		return ConstIterator(*this, size);
	}

	//! resize the internal list
	//! @param len number of elements to size the list for
	//! @return *this
	virtual ArrayList& alloc(Uint32 len) {
		maxSize = len;
		Uint32 newSize = std::min(maxSize, size);
		T* newArr = nullptr;
		if (len) {
			newArr = new T[len];
			assert(newArr);
			Uint32 copyLen = min(size, len);
			for (Uint32 c = 0; c < copyLen; ++c) {
				newArr[c] = std::move(arr[c]);
			}
		}
		if (arr) {
			delete[] arr;
			arr = nullptr;
		}
		size = newSize;
		arr = newArr;
		return *this;
	}

	//! fill the internal list, resizing if necessary
	//! @param len number of elements to size the list for
	//! @return *this
	ArrayList& resize(Uint32 len) {
		if (len > maxSize) {
			alloc(len);
		}
		if (len > size) {
			for (Uint32 c = size; c < len; ++c) {
				arr[c] = T();
			}
		}
		size = len;
		return *this;
	}

	//! empty the list
	//! @return *this
	ArrayList& clear() {
		alloc(0);
		return *this;
	}

	//! replace list contents with those of another list
	//! @param src the list to copy into our list
	//! @return *this;
	ArrayList& copy(const ArrayList& src) {
		alloc(src.getSize());
		size = src.getSize();
		for (Uint32 c = 0; c < src.getSize(); ++c) {
			arr[c] = src[c];
		}
		return *this;
	}

	//! replace list contents with those of an array
	//! @param src the array to copy into our list
	//! @return *this;
	ArrayList& copy(const std::initializer_list<T>& src) {
		alloc(static_cast<Uint32>(src.size()));
		size = static_cast<Uint32>(src.size());
		const T* it;
		Uint32 c;
		for (c = 0, it = std::begin(src); it != std::end(src); ++it, ++c) {
			arr[c] = *it;
		}
		return *this;
	}

	//! quickly swap the internal array of this list with that of another list
	//! @param src the list to swap with
	//! @return *this;
	virtual ArrayList<T>& swap(ArrayList<T>&& src) {
		auto tarr = src.arr;
		auto tsize = src.size;
		auto tmaxSize = src.maxSize;
		src.arr = arr;
		src.size = size;
		src.maxSize = maxSize;
		arr = tarr;
		size = tsize;
		maxSize = tmaxSize;
		return *this;
	}

	//! push a value onto the list
	//! @param val the value to push
	void push(const T& val) {
		if (size == maxSize) {
			alloc(std::max((unsigned int)size * 2U, 4U));
		}
		++size;
		arr[size - 1] = std::move(val);
	}

	//! insert a value into the list
	//! @param val the value to insert
	//! @param pos the index to displace (move to the end of the list)
	void insert(const T& val, Uint32 pos) {
		assert(pos <= size);
		if (size == maxSize) {
			alloc(std::max((unsigned int)size * 2U, 4U));
		}
		++size;
		arr[size - 1] = arr[pos];
		arr[pos] = val;
	}

	//! insert a value into the list, rearranging all elements after it
	//! @param val the value to insert
	//! @param pos the index to displace (move all elements starting here 1 index forward)
	void insertAndRearrange(const T& val, Uint32 pos) {
		assert(pos <= size);
		if (size == maxSize) {
			alloc(std::max((unsigned int)size * 2U, 4U));
		}
		++size;
		for (Uint32 c = size - 1; c > pos; --c) {
			arr[c] = arr[c - 1];
		}
		arr[pos] = val;
	}

	//! removes and returns the last element from the list
	//! @return the element
	T pop() {
		assert(size > 0);
		--size;
		return arr[size];
	}

	//! returns the last element in the list without removing it
	//! @return the element
	const T& peek() const {
		assert(size > 0);
		return arr[size - 1];
	}

	//! returns the last element in the list without removing it
	//! @return the element
	T& peek() {
		assert(size > 0);
		return arr[size - 1];
	}

	//! removes and returns an element from the list without moving the rest of the list
	//! @param pos the index of the element to remove
	//! @return the value at the given index
	T remove(Uint32 pos) {
		assert(size > pos);
		T result = arr[pos];
		--size;
		arr[pos] = arr[size];
		return result;
	}

	//! removes and returns an element from the list, rearranging all elements after it
	//! @param pos the index of the element to remove
	//! @return the value at the given index
	T removeAndRearrange(Uint32 pos) {
		assert(size > pos);
		T result = arr[pos];

		Uint32 newSize = size - 1;
		for (Uint32 c = pos; c < newSize; ++c) {
			arr[c] = arr[c + 1];
		}

		--size;
		return result;
	}

	//! replace list contents with those of another list
	//! @param src the list to copy into our list
	//! @return *this;
	ArrayList<T>& operator=(const ArrayList& src) {
		return copy(src);
	}

	//! exchange contents with another list
	//! @param src the list to copy into our list
	//! @return *this;
	ArrayList<T>& operator=(ArrayList&& src) {
		return swap(std::move(src));
	}

	//! replace list contents with those of an array
	//! @param src the array to copy into our list
	//! @return *this;
	ArrayList<T>& operator=(const std::initializer_list<T>& src) {
		return copy(src);
	}

	//! get list contents at specified index
	//! @param pos index value
	//! @return a reference to the list at this index
	const T get(int pos) const {
		assert((Uint32)pos < size);
		return arr[(Uint32)pos];
	}

	//! get list contents at specified index
	//! @param pos index value
	//! @return a reference to the list at this index
	T get(int pos) {
		assert((Uint32)pos < size);
		return arr[(Uint32)pos];
	}

	//! get list contents at specified index
	//! @param pos index value
	//! @return a reference to the list at this index
	const T& operator[](Uint32 pos) const {
		assert(pos < size);
		return arr[pos];
	}

	//! get list contents at specified index
	//! @param pos index value
	//! @return a reference to the list at this index
	T& operator[](Uint32 pos) {
		assert(pos < size);
		return arr[pos];
	}

	//! abstract class to define sort function
	class SortFunction {
	public:
		SortFunction() {}
		virtual ~SortFunction() {}

		//! compare a and b
		//! @param a the first element to compare
		//! @param b the second element to compare
		//! @return true if a should be placed before b
		virtual const bool operator()(const T a, const T b) const = 0;
	};

	//! sort the array list using the given function
	//! @param fn The sort function to use
	void sort(const SortFunction& fn) {
		if (size <= 1u) {
			return;
		}
		mergeSortImp(fn, 0U, size - 1U);
	}

	//! exposes this list type to a script
	//! @param lua The script engine to expose to
	//! @param name The type name in lua
	static void exposeToScript(lua_State* lua, const char* name) {
		typedef T* (ArrayList<T>::*ArrayFn)();
		ArrayFn getArray = static_cast<ArrayFn>(&ArrayList<T>::getArray);

		typedef const T* (ArrayList<T>::*ArrayConstFn)() const;
		ArrayConstFn getArrayConst = static_cast<ArrayConstFn>(&ArrayList<T>::getArray);

		typedef ArrayList<T>& (ArrayList<T>::*CopyFn)(const ArrayList<T>&);
		CopyFn copy = static_cast<CopyFn>(&ArrayList<T>::copy);

		typedef T& (ArrayList<T>::*PeekFn)();
		PeekFn peek = static_cast<PeekFn>(&ArrayList<T>::peek);

		typedef const T& (ArrayList<T>::*PeekConstFn)() const;
		PeekConstFn peekConst = static_cast<PeekConstFn>(&ArrayList<T>::peek);

		typedef T(ArrayList<T>::*GetFn)(int);
		GetFn get = static_cast<GetFn>(&ArrayList<T>::get);

		typedef const T(ArrayList<T>::*GetConstFn)(int) const;
		GetConstFn getConst = static_cast<GetConstFn>(&ArrayList<T>::get);

		luabridge::getGlobalNamespace(lua)
			.template beginClass<ArrayList<T>>(name)
			.template addConstructor<void (*)()>()
			.template addFunction("getArray", getArray)
			.template addFunction("getArrayConst", getArrayConst)
			.template addFunction("getSize", &ArrayList<T>::getSize)
			.template addFunction("getMaxSize", &ArrayList<T>::getMaxSize)
			.template addFunction("empty", &ArrayList<T>::empty)
			.template addFunction("alloc", &ArrayList<T>::alloc)
			.template addFunction("resize", &ArrayList<T>::resize)
			.template addFunction("clear", &ArrayList<T>::clear)
			.template addFunction("copy", copy)
			.template addFunction("push", &ArrayList<T>::push)
			.template addFunction("insert", &ArrayList<T>::insert)
			.template addFunction("pop", &ArrayList<T>::pop)
			.template addFunction("peek", peek)
			.template addFunction("peekConst", peekConst)
			.template addFunction("remove", &ArrayList<T>::remove)
			.template addFunction("removeAndRearrange", &ArrayList<T>::removeAndRearrange)
			.template addFunction("get", get)
			.template addFunction("getConst", getConst)
			.template endClass()
			;
	}

protected:
	T* arr = nullptr;		//! array data
	Uint32 size = 0;		//! current array capacity
	Uint32 maxSize = 0;		//! maximum array capacity

	//! recursive merge sort algorithm
	//! @param fn the sort function to test with
	//! @param l the left index of the subarray to sort
	//! @param r the right index of the subarray to sort
	void mergeSortImp(const SortFunction& fn, Uint32 l, Uint32 r) {
		if (l < r) {
			// same as (l+r) / 2, but avoids overflow for large l and r
			Uint32 m = l + (r - l) / 2U;

			// sort first and second halves
			mergeSortImp(fn, l, m);
			mergeSortImp(fn, m + 1U, r);

			merge(fn, l, m, r);
		}
	}

	//! merge two subarrays within the array
	//! @param fn the sort function to test with
	//! @param l the start of the first subarray
	//! @param m the midpoint
	//! @param r the end of the right subarray
	void merge(const SortFunction& fn, Uint32 l, Uint32 m, Uint32 r) {
		Uint32 n1 = m - l + 1U; // length of first subarray
		Uint32 n2 = r - m;      // length of second subarray

		// create temp vectors
		ArrayList<T> L, R;
		L.resize(n1);
		R.resize(n2);

		// copy data to temp vectors L and R
		for (Uint32 i = 0U; i < n1; ++i) {
			L[i] = arr[l + i];
		}
		for (Uint32 i = 0U; i < n2; ++i) {
			R[i] = arr[m + 1U + i];
		}

		// merge the temp vectors back into arr[l..r]
		Uint32 i = 0U;  // initial index of first subarray
		Uint32 j = 0U;  // initial index of second subarray
		Uint32 k = l;   // initial index of merged subarray
		while (i < n1 && j < n2) {
			if (fn(L[i], R[j])) {
				arr[k] = L[i];
				++i;
			} else {
				arr[k] = R[j];
				++j;
			}
			++k;
		}

		// copy the remaining elements of L, if there are any
		while (i < n1) {
			arr[k] = L[i];
			++i;
			++k;
		}

		// copy the remaining elements of R, if there are any
		while (j < n2) {
			arr[k] = R[j];
			++j;
			++k;
		}
	}
};

//! A Static array list is allocated on the stack instead of the heap.
template <typename T, Uint32 defaultSize>
class StaticArrayList : public ArrayList<T> {
public:
	StaticArrayList() = default;

	StaticArrayList(const StaticArrayList& src) {
		copy(src);
	}

	StaticArrayList(StaticArrayList&& src) = delete;

	StaticArrayList(const std::initializer_list<T>& src) {
		copy(src);
	}

	virtual ~StaticArrayList() {
		if (this->arr == defaultArr) {
			this->arr = nullptr;
		}
	}

	StaticArrayList<T, defaultSize>& operator=(const StaticArrayList<T, defaultSize>& rhs) {
		return copy(rhs);
	}

	StaticArrayList<T, defaultSize>& operator=(StaticArrayList<T, defaultSize>&&) = delete;

	virtual ArrayList<T>& swap(ArrayList<T>&& src) override {
		assert(0 && "StaticArrayList::swap not supported");
		return *this;
	}

	//! resize the internal list
	//! @param len number of elements to size the list for
	//! @return *this
	virtual ArrayList<T>& alloc(Uint32 len) override {
		assert(0 && "StaticArrayList::alloc not supported");
		return *this;
	}

	static Uint32 getDefaultSize() { return defaultSize; }

protected:
	T defaultArr[defaultSize];
};
