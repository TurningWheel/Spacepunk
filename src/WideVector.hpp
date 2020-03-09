// WideVector.hpp
// Records (x, y, z, w) vector with float accuracy

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#include "Vector.hpp"

class WideVector : public Vector {
public:
	float w = 0.f;

	// constructors
	WideVector() {}
	WideVector(const WideVector& src) {
		x = src.x;
		y = src.y;
		z = src.z;
		w = src.w;
	}
	WideVector(const float factor) {
		x = factor;
		y = factor;
		z = factor;
		w = factor;
	}
	WideVector(float _x, float _y, float _z, float _w) {
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
	WideVector(float* arr) {
		x = arr[0];
		y = arr[1];
		z = arr[2];
		w = arr[3];
	}
	~WideVector() {}

	// determines whether the vector represents a true volume
	// @return true if all elements are nonzero, false otherwise
	bool hasVolume() const {
		if (x == 0.f || y == 0.f || z == 0.f || w == 0.f) {
			return false;
		}
		return true;
	}

	// calculate dot product of this vector and another
	// @return the dot product of the two vectors
	float dot(const WideVector& other) const {
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	// calculate cross product of this vector and another
	// @return the cross product of the two vectors
	WideVector cross(const WideVector& other) const {
		WideVector result;
		result.x = y * other.z - z * other.y;
		result.y = z * other.w - w * other.z;
		result.z = w * other.x - x * other.w;
		result.w = x * other.y - y * other.x;
		return result;
	}

	// calculate the length of the vector (uses sqrt)
	// @return the length
	float length() const {
		return sqrtf(x*x + y * y + z * z + w * w);
	}

	// calculate the length of the vector, sans sqrt
	// @return the length squared
	float lengthSquared() const {
		return x * x + y * y + z * z + w * w;
	}

	// create a normalized copy of this vector
	// @return a normalized version of this vector
	WideVector normal() const {
		float l = length();
		return WideVector(x / l, y / l, z / l, w / l);
	}

	// normalize this vector
	void normalize() {
		float l = length();
		x /= l;
		y /= l;
		z /= l;
		w /= l;
	}

	// conversion to glm::vec4
	operator glm::vec4() const {
		return glm::vec4(x, y, z, w);
	}

	// conversion to btVector4
	operator btVector4() const {
		return btVector4(x, y, z, w);
	}

	// operator =
	WideVector& operator=(const WideVector& src) {
		x = src.x;
		y = src.y;
		z = src.z;
		w = src.w;
		return *this;
	}

	// operator +
	WideVector operator+(const WideVector& src) const {
		WideVector result;
		result.x = x + src.x;
		result.y = y + src.y;
		result.z = z + src.z;
		result.w = w + src.w;
		return result;
	}

	// operator +=
	WideVector& operator+=(const WideVector& src) {
		x += src.x;
		y += src.y;
		z += src.z;
		w += src.w;
		return *this;
	}

	// operator -
	WideVector operator-(const WideVector& src) const {
		WideVector result;
		result.x = x - src.x;
		result.y = y - src.y;
		result.z = z - src.z;
		result.w = w - src.w;
		return result;
	}

	// operator -=
	WideVector& operator-=(const WideVector& src) {
		x -= src.x;
		y -= src.y;
		z -= src.z;
		w -= src.w;
		return *this;
	}

	// operator *
	WideVector operator*(const WideVector& src) const {
		WideVector result;
		result.x = x * src.x;
		result.y = y * src.y;
		result.z = z * src.z;
		result.w = w * src.w;
		return result;
	}
	WideVector operator*(const float& src) const {
		WideVector result;
		result.x = x * src;
		result.y = y * src;
		result.z = z * src;
		result.w = w * src;
		return result;
	}

	// operator *=
	WideVector& operator*=(const WideVector& src) {
		x *= src.x;
		y *= src.y;
		z *= src.z;
		w *= src.w;
		return *this;
	}
	WideVector& operator*=(const float& src) {
		x *= src;
		y *= src;
		z *= src;
		w *= src;
		return *this;
	}

	// operator /
	WideVector operator/(const WideVector& src) const {
		WideVector result;
		result.x = x / src.x;
		result.y = y / src.y;
		result.z = z / src.z;
		result.w = w / src.w;
		return result;
	}
	WideVector operator/(const float& src) const {
		WideVector result;
		result.x = x / src;
		result.y = y / src;
		result.z = z / src;
		result.w = w / src;
		return result;
	}

	// operator /=
	WideVector& operator/=(const WideVector& src) {
		x /= src.x;
		y /= src.y;
		z /= src.z;
		w /= src.w;
		return *this;
	}
	WideVector& operator/=(const float& src) {
		x /= src;
		y /= src;
		z /= src;
		w /= src;
		return *this;
	}

	// CONDITIONAL OPERATORS

	// operator >
	bool operator>(const WideVector& other) const {
		if (lengthSquared() > other.lengthSquared()) {
			return true;
		} else {
			return false;
		}
	}

	// operator >=
	bool operator>=(const WideVector& other) const {
		if (lengthSquared() >= other.lengthSquared()) {
			return true;
		} else {
			return false;
		}
	}

	// operator ==
	bool operator==(const WideVector& other) const {
		if (x == other.x && y == other.y && z == other.z && w == other.w) {
			return true;
		} else {
			return false;
		}
	}

	// operator <=
	bool operator<=(const WideVector& other) const {
		if (lengthSquared() <= other.lengthSquared()) {
			return true;
		} else {
			return false;
		}
	}

	// operator <
	bool operator<(const WideVector& other) const {
		if (lengthSquared() < other.lengthSquared()) {
			return true;
		} else {
			return false;
		}
	}

	// operator !=
	bool operator!=(const WideVector& other) const {
		if (x != other.x || y != other.y || z != other.z || w != other.w) {
			return true;
		} else {
			return false;
		}
	}
};