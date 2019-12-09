// Quaternion.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bullet3/LinearMath/btQuaternion.h>

#include "File.hpp"

class Quaternion {
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 1.f;

	Quaternion() {
	}

	Quaternion(float _x, float _y, float _z, float _w) :
		x(_x),
		y(_y),
		z(_z),
		w(_w)
	{}

	Quaternion(const Quaternion& q) :
		x(q.x),
		y(q.y),
		z(q.z),
		w(q.w)
	{}

	Quaternion(const Rotation& rot) {
		*this = rotate(rot);
	}

	Quaternion(const glm::mat4& mat) {
		glm::mat4 m = mat;
		m[0] /= m[0].length();
		m[1] /= m[1].length();
		m[2] /= m[2].length();
		glm::quat quat(mat);
		x = quat.x;
		y = quat.y;
		z = quat.z;
		w = quat.w;
	}

	Quaternion rotate(const Rotation& rot) const {
		if (rot.yaw == 0.f && rot.pitch == 0.f && rot.roll == 0.f) {
			return *this;
		}

		const float YawNoWinding = fmodf(rot.yaw, PI * 2.f);
		const float PitchNoWinding = fmodf(rot.pitch, PI * 2.f);
		const float RollNoWinding = fmodf(rot.roll, PI * 2.f);

		float SP, SY, SR;
		float CP, CY, CR;
		SY = sinf(YawNoWinding / 2.f); CY = cosf(YawNoWinding / 2.f);
		SP = sinf(PitchNoWinding / 2.f); CP = cosf(PitchNoWinding / 2.f);
		SR = sinf(RollNoWinding / 2.f); CR = cosf(RollNoWinding / 2.f);

		Quaternion q;
		q.x =  CR*SP*SY - SR*CP*CY; // z
		q.z = -CR*SP*CY - SR*CP*SY; // x
		q.y = -CR*CP*SY + SR*SP*CY; // y
		q.w =  CR*CP*CY + SR*SP*SY;
		return q * *this;
	}

	Rotation toRotation() const {
		// reference:
		// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

		Rotation result;

		const float singularityTest = x*y + z*w;
		const float epsilon = 0.4999995f;

		result.pitch = -asinf(2.f*x*y + 2.f*z*w);
		if (singularityTest < -epsilon) {
			result.yaw = 2.f * atan2f(x, w);
			result.roll = 0.f;
		} else if (singularityTest > epsilon) {
			result.yaw = -2.f * atan2f(x, w);
			result.roll = 0.f;
		} else {
			result.yaw = -atan2f(2.f*y*w - 2.f*x*z, 1.f - 2.f*y*y - 2.f*z*z);
			result.roll = -atan2f(2.f*x*w - 2.f*y*z, 1.f - 2.f*x*x - 2.f*z*z);
		}

		result.yaw = (result.yaw == -0.f) ? 0.f : result.yaw;
		result.pitch = (result.pitch == -0.f) ? 0.f : result.pitch;
		result.roll = (result.roll == -0.f) ? 0.f : result.roll;

		return result;
	}

	Vector toVector() const {
		/*Vector v(1.f, 0.f, 0.f);
		const Vector q(x, y, z);
		const Vector t = 2.f * q.cross(v);
		const Vector result = v + (w * t) + q.cross(t);
		return result;*/
		Rotation r = toRotation();
		return r.toVector();
	}

	Quaternion& operator*=(const Quaternion& q) {
		Quaternion o(*this);
		x = o.w * q.x + o.x * q.w + o.y * q.z - o.z * q.y;
		y = o.w * q.y + o.y * q.w + o.z * q.x - o.x * q.z;
		z = o.w * q.z + o.z * q.w + o.x * q.y - o.y * q.x;
		w = o.w * q.w - o.x * q.x - o.y * q.y - o.z * q.z;
		return *this;
	}

	Quaternion operator*(const Quaternion& q) const {
		Quaternion result;
		Quaternion o(*this);
		result.x = o.w * q.x + o.x * q.w + o.y * q.z - o.z * q.y;
		result.y = o.w * q.y + o.y * q.w + o.z * q.x - o.x * q.z;
		result.z = o.w * q.z + o.z * q.w + o.x * q.y - o.y * q.x;
		result.w = o.w * q.w - o.x * q.x - o.y * q.y - o.z * q.z;
		return result;
	}

	bool operator==(const Quaternion& q) const {
		return x==q.x && y==q.y && z==q.z && w==q.w;
	}
	
	bool operator!=(const Quaternion& q) const {
		return x!=q.x || y!=q.y || z!=q.z || w!=q.w;
	}

	void serialize(FileInterface * file) {
		int version = 0;
		file->property("Quaternion::version", version);
		file->property("x", x);
		file->property("y", y);
		file->property("z", z);
		file->property("w", w);
	}
};

inline btQuaternion btQuat(const Quaternion& q) {
	Rotation r = q.toRotation();
	btQuaternion btQ;
	btQ.setEulerZYX(r.yaw, -r.pitch, -r.roll);
	return btQ;
	//return btQuaternion(-q.x, q.z, -q.y, -q.w);
}

//return btQuaternion(q.z, q.y, -q.x, -q.w);