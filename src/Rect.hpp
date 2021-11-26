//! @file Rect.hpp

#pragma once

#include "File.hpp"

//! Defines a two-dimensional rectangle
template <typename T>
struct Rect {
	T x = 0, y = 0, w = 0, h = 0;

	Rect() = default;
	Rect(const Rect&) = default;
	Rect(Rect&&) = default;
	~Rect() = default;

	Rect& operator=(const Rect&) = default;
	Rect& operator=(Rect&&) = default;

	Rect(T _x, T _y, T _w, T _h) {
		x = _x;
		y = _y;
		w = _w;
		h = _h;
	}

	Rect(T* arr) {
		x = arr[0];
		y = arr[1];
		w = arr[2];
		h = arr[3];
	}

	//! determines if the given point lies within the bounds of the rectangle
	//! @param pointX the x coordinate of the point to test
	//! @param pointY the y coordinate of the point to test
	//! @return true if the point is within the rectangle, and false if it is not
	bool containsPoint(T pointX, T pointY) const {
		if (pointX >= x && pointX < x + w &&
			pointY >= y && pointY < y + h) {
			return true;
		} else {
			return false;
		}
	}

	//! save/load this object to a file
	//! @param file interface to serialize with
	void serialize(FileInterface * file) {
		file->property("x", x);
		file->property("y", y);
		file->property("w", w);
		file->property("h", h);
	}

	//! exposes this rect type to a script
	//! @param lua The script engine to expose to
	//! @param name The type name in lua
	static void exposeToScript(lua_State* lua, const char* name) {
		luabridge::getGlobalNamespace(lua)
			.template beginClass<Rect<T>>(name)
			.template addConstructor<void (*)(T, T, T, T)>()
			.template addProperty("x", &Rect<T>::x, true)
			.template addProperty("y", &Rect<T>::y, true)
			.template addProperty("w", &Rect<T>::w, true)
			.template addProperty("h", &Rect<T>::h, true)
			.template addFunction("containsPoint", &Rect<T>::containsPoint)
			.template endClass()
			;
	}
};
