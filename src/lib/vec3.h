
#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>

#include "log.h"
#include "vec2.h"

struct Vec3 {

	Vec3() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}
	explicit Vec3(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	}
	explicit Vec3(int32_t _x, int32_t _y, int32_t _z) {
		x = static_cast<float>(_x);
		y = static_cast<float>(_y);
		z = static_cast<float>(_z);
	}
	explicit Vec3(float f) {
		x = y = z = f;
	}

	Vec3(const Vec3&) = default;
	Vec3& operator=(const Vec3&) = default;
	~Vec3() = default;

	float& operator[](uint32_t idx) {
		assert(idx <= 2);
		return data[idx];
	}
	float operator[](uint32_t idx) const {
		assert(idx <= 2);
		return data[idx];
	}

	Vec3 operator+=(Vec3 v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	Vec3 operator-=(Vec3 v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	Vec3 operator*=(Vec3 v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	Vec3 operator/=(Vec3 v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	Vec3 operator+=(float s) {
		x += s;
		y += s;
		z += s;
		return *this;
	}
	Vec3 operator-=(float s) {
		x -= s;
		y -= s;
		z -= s;
		return *this;
	}
	Vec3 operator*=(float s) {
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}
	Vec3 operator/=(float s) {
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}

	Vec3 operator+(Vec3 v) const {
		return Vec3(x + v.x, y + v.y, z + v.z);
	}
	Vec3 operator-(Vec3 v) const {
		return Vec3(x - v.x, y - v.y, z - v.z);
	}
	Vec3 operator*(Vec3 v) const {
		return Vec3(x * v.x, y * v.y, z * v.z);
	}
	Vec3 operator/(Vec3 v) const {
		return Vec3(x / v.x, y / v.y, z / v.z);
	}

	Vec3 operator+(float s) const {
		return Vec3(x + s, y + s, z + s);
	}
	Vec3 operator-(float s) const {
		return Vec3(x - s, y - s, z - s);
	}
	Vec3 operator*(float s) const {
		return Vec3(x * s, y * s, z * s);
	}
	Vec3 operator/(float s) const {
		return Vec3(x / s, y / s, z / s);
	}

	bool operator==(Vec3 v) const {
		return x == v.x && y == v.y && z == v.z;
	}
	bool operator!=(Vec3 v) const {
		return x != v.x || y != v.y || z != v.z;
	}

	/// Absolute value
	Vec3 abs() const {
		return Vec3(std::abs(x), std::abs(y), std::abs(z));
	}
	/// Negation
	Vec3 operator-() const {
		return Vec3(-x, -y, -z);
	}
	/// Are all members real numbers?
	bool valid() const {
		return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
	}

	/// Modify vec to have unit length
	Vec3 normalize() {
		float n = norm();
		x /= n;
		y /= n;
		z /= n;
		return *this;
	}
	/// Return unit length vec in the same direction
	Vec3 unit() const {
		float n = norm();
		return Vec3(x / n, y / n, z / n);
	}

	float norm_squared() const {
		return x * x + y * y + z * z;
	}
	float norm() const {
		return std::sqrt(norm_squared());
	}
	
	/// Returns first two components
	Vec2 xy() const {
		return Vec2(x, y);
	}

	/// Make sure all components are in the range [min,max) with floating point mod logic
	Vec3 range(float min, float max) const {
		if (!valid()) return Vec3();
		Vec3 r = *this;
		float range = max - min;
		while (r.x < min) r.x += range;
		while (r.x >= max) r.x -= range;
		while (r.y < min) r.y += range;
		while (r.y >= max) r.y -= range;
		while (r.z < min) r.z += range;
		while (r.z >= max) r.z -= range;
		return r;
	}

	union {
		struct {
			float x;
			float y;
			float z;
		};
		float data[3] = {};
	};
};

inline Vec3 operator+(float s, Vec3 v) {
	return Vec3(v.x + s, v.y + s, v.z + s);
}
inline Vec3 operator-(float s, Vec3 v) {
	return Vec3(v.x - s, v.y - s, v.z - s);
}
inline Vec3 operator*(float s, Vec3 v) {
	return Vec3(v.x * s, v.y * s, v.z * s);
}
inline Vec3 operator/(float s, Vec3 v) {
	return Vec3(s / v.x, s / v.y, s / v.z);
}

/// Take minimum of each component
inline Vec3 hmin(Vec3 l, Vec3 r) {
	return Vec3(std::min(l.x, r.x), std::min(l.y, r.y), std::min(l.z, r.z));
}

/// Take maximum of each component
inline Vec3 hmax(Vec3 l, Vec3 r) {
	return Vec3(std::max(l.x, r.x), std::max(l.y, r.y), std::max(l.z, r.z));
}

/// 3D dot product
inline float dot(Vec3 l, Vec3 r) {
	return l.x * r.x + l.y * r.y + l.z * r.z;
}

/// 3D cross product
inline Vec3 cross(Vec3 l, Vec3 r) {
	return Vec3(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
}

inline std::string to_string(Vec3 const &v) {
	return "{" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + "}";
}

inline std::ostream& operator<<(std::ostream& out, Vec3 v) {
	out << "{" << v.x << "," << v.y << "," << v.z << "}";
	return out;
}

inline bool operator<(Vec3 l, Vec3 r) {
	if (l.x == r.x) {
		if (l.y == r.y) {
			return l.z < r.z;
		}
		return l.y < r.y;
	}
	return l.x < r.x;
}
