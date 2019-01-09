// Rect.hpp
// Defines a two-dimensional rectangle

#pragma once

#include "File.hpp"
#include "Script.hpp"

template <typename T>
struct Rect {
	T x=0, y=0, w=0, h=0;

	Rect() {};

	Rect( T _x, T _y, T _w, T _h ) {
		x = _x;
		y = _y;
		w = _w;
		h = _h;
	}

	Rect( T* arr ) {
		x = arr[0];
		y = arr[1];
		w = arr[2];
		h = arr[3];
	}

	Rect( const Rect<T>& rect ) {
		*this = rect;
	}

	// determines if the given point lies within the bounds of the rectangle
	// @param pointX the x coordinate of the point to test
	// @param pointY the y coordinate of the point to test
	// @return true if the point is within the rectangle, and false if it is not
	bool containsPoint( T pointX, T pointY ) const {
		if( pointX >= x && pointX < x+w &&
			pointY >= y && pointY < y+h ) {
			return true;
		} else {
			return false;
		}
	}

	// save/load this object to a file
	// @param file interface to serialize with
	void serialize(FileInterface * file) {
		file->property("x", x);
		file->property("y", y);
		file->property("w", w);
		file->property("h", h);
	}

	// exposes this rect type to a script
	// @param lua The script engine to expose to
	// @param luaTypeName The type name in lua
	static void exposeToScript(sol::state& lua, const char* luaTypeName)
	{
		//First identify the constructors.
		sol::constructors<Rect(), Rect(T, T, T, T), Rect(T*), Rect(const Rect<T>&)> constructors;

		//Then do the thing.
		sol::usertype<Rect<T>> usertype(constructors,
			"x", &Rect<T>::x,
			"y", &Rect<T>::y,
			"w", &Rect<T>::w,
			"h", &Rect<T>::h,
			"containsPoint", &Rect<T>::containsPoint
		);

		//Finally register the thing.
		lua.set_usertype(luaTypeName, usertype);
	}
};