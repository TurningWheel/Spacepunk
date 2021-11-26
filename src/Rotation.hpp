//! @file Rotation.hpp

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "Main.hpp"
#include "File.hpp"
#include "Vector.hpp"

//! Records Euler angles in radians (convertible to degrees)
class Rotation {
public:
	float yaw = 0, pitch = 0, roll = 0;

	Rotation() = default;
	Rotation(const Rotation& src) = default;
	Rotation(Rotation&&) = default;
	Rotation(float _yaw, float _pitch, float _roll) {
		yaw = _yaw;
		pitch = _pitch;
		roll = _roll;
	}
	~Rotation() = default;

	Rotation& operator=(const Rotation&) = default;
	Rotation& operator=(Rotation&&) = default;

	//! converts the angle attributes to radians (default)
	float		radiansYaw() const { return yaw; }
	float		radiansPitch() const { return pitch; }
	float		radiansRoll() const { return roll; }

	//! converts the angle attributes to degrees
	float		degreesYaw() const { return (yaw * radiansToDegrees); }
	float		degreesPitch() const { return (pitch * radiansToDegrees); }
	float		degreesRoll() const { return (roll * radiansToDegrees); }

	//! add one angle to another
	Rotation operator+(const Rotation& src) const {
		Rotation result;
		result.yaw = yaw + src.yaw;
		result.pitch = pitch + src.pitch;
		result.roll = roll + src.roll;
		return result;
	}

	//! add one angle to another
	Rotation& operator+=(const Rotation& src) {
		yaw += src.yaw;
		pitch += src.pitch;
		roll += src.roll;
		return *this;
	}

	//! subtract one angle from another
	Rotation operator-(const Rotation& src) const {
		Rotation result;
		result.yaw = yaw - src.yaw;
		result.pitch = pitch - src.pitch;
		result.roll = roll - src.roll;
		return result;
	}

	//! subtract one angle from another
	Rotation& operator-=(const Rotation& src) {
		yaw -= src.yaw;
		pitch -= src.pitch;
		roll -= src.roll;
		return *this;
	}

	//! formats the values of yaw, pitch, and roll so they lie between -PI*2 (exclusive) and PI*2 (exclusive)
	Rotation wrapAngles() {
		yaw = fmod(yaw, PI * 2);
		pitch = fmod(pitch, PI * 2);
		roll = fmod(roll, PI * 2);
		return *this;
	}

	//! wraps the angles, then binds them to the range 0 (inclusive) and PI*2 (exclusive)
	Rotation bindAngles() {
		wrapAngles();
		if (yaw < 0)
			yaw += PI * 2;
		if (pitch < 0)
			pitch += PI * 2;
		if (roll < 0)
			roll += PI * 2;
		if (yaw >= PI * 2)
			yaw -= PI * 2;
		if (pitch >= PI * 2)
			pitch -= PI * 2;
		if (roll >= PI * 2)
			roll -= PI * 2;
		return *this;
	}

	//! converts the euler angle to a direction vector
	Vector toVector() const {
		glm::mat4 matrix = glm::yawPitchRoll(yaw, roll, pitch);
		glm::vec3 vector(matrix * glm::vec4(1.f, 0.f, 0.f, 0.f));
		return Vector(vector.x, -vector.z, vector.y);
	}

	//! operator ==
	bool operator==(const Rotation& other) const {
		if (yaw == other.yaw && pitch == other.pitch && roll == other.roll) {
			return true;
		} else {
			return false;
		}
	}

	//! operator !=
	bool operator!=(const Rotation& other) const {
		if (yaw != other.yaw || pitch != other.pitch || roll != other.roll) {
			return true;
		} else {
			return false;
		}
	}

	//! save/load this object to a file
	//! @param file interface to serialize with
	void serialize(FileInterface * file) {
		file->property("yaw", yaw);
		file->property("pitch", pitch);
		file->property("roll", roll);
	}

	static const float radiansToDegrees;
};
