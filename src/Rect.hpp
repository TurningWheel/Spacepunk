// Rect.hpp
// Defines a two-dimensional rectangle

#pragma once

#include "File.hpp"

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
	// @param pointX: the x coordinate of the point to test
	// @param pointY: the y coordinate of the point to test
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
		file->property("y", x);
		file->property("w", x);
		file->property("h", x);
	}
};