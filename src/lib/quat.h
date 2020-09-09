
#pragma once

#include <cmath>
#include <algorithm>

#include "log.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4.h"

struct Quat {

	Quat() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
	}
	explicit Quat(float _x, float _y, float _z, float _w) {
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
	explicit Quat(Vec3 complex, float real) {
		x = complex.x;
		y = complex.y;
		z = complex.z;
		w = real;
	}
	explicit Quat(const Vec4& src) {
		x = src.x;
		y = src.y;
		z = src.z;
		w = src.w;
	}
	Quat(const Quat& src) {
		x = src.x;
		y = src.y;
		z = src.z;
		w = src.w;
	}
	~Quat() = default;

	/// Create unit quaternion representing given axis-angle rotation
	static Quat axis_angle(Vec3 axis, float angle) {
		axis.normalize();
		angle = Radians(angle) / 2.0f;
		float sin = std::sin(angle);
		float x = sin * axis.x;
		float y = sin * axis.y;
		float z = sin * axis.z;
		float w = std::cos(angle);
		return Quat(x,y,z,w).unit();
	}
	
	/// Create unit quaternion representing given euler angles (XYZ)
	static Quat euler(Vec3 angles) {
		if(angles == Vec3{0.0f,0.0f,180.0f} || angles == Vec3{180.0f,0.0f,0.0f}) 
			return Quat{0.0f,0.0f,-1.0f,0.0f};
		float c1 = std::cos(Radians(angles[2] * 0.5f));
		float c2 = std::cos(Radians(angles[1] * 0.5f));
		float c3 = std::cos(Radians(angles[0] * 0.5f));
		float s1 = std::sin(Radians(angles[2] * 0.5f));
		float s2 = std::sin(Radians(angles[1] * 0.5f));
		float s3 = std::sin(Radians(angles[0] * 0.5f));
		float x = c1*c2*s3 - s1*s2*c3;
		float y = c1*s2*c3 + s1*c2*s3;
		float z = s1*c2*c3 - c1*s2*s3;
		float w = c1*c2*c3 + s1*s2*s3;
		return Quat(x,y,z,w);
	}

	Quat operator=(const Quat& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	float& operator[](int idx) {
		assert(idx >= 0 && idx <= 3);
		return data[idx];
	}
	float operator[](int idx) const {
		assert(idx >= 0 && idx <= 3);
		return data[idx];
	}

	Quat conjugate() const {
		return Quat(-x, -y, -z, w);
	}
	Quat inverse() const {
		return conjugate().unit();
	}
	Vec3 complex() const {
		return Vec3(x,y,z);
	}
	float real() const {
		return w;
	}
	
	float norm_squared() const {
		return x * x + y * y + z * z + w * w;
	}
	float norm() const {
		return std::sqrt(norm_squared());
	}
	Quat unit() const {
		float n = norm();
		return Quat(x / n, y / n, z / n, w / n);
	}

	Quat operator*(const Quat& r) const {
		return Quat(y*r.z - z*r.y + x*r.w + w*r.x,
					z*r.x - x*r.z + y*r.w + w*r.y,
					x*r.y - y*r.x + z*r.w + w*r.z,
					w*r.w - x*r.x - y*r.y - z*r.z);
	}

	Quat operator+(const Quat& r) const {
		return Quat(x+r.x,y+r.y,z+r.z,w+r.w);
	}
	Quat operator-(const Quat& r) const {
		return Quat(x-r.x,y-r.y,z-r.z,w-r.w);
	}

	void scale(float f) {
		x *= f;
		y *= f;
		z *= f;
		w *= f;
	}
	void make_log() {
		float a0 = w;
		w = 0.0f;
		if(std::abs(a0) < 1.0f) {
			float angle = std::acos(a0);
			float sin_angle = std::sin(angle);
			if(std::abs(sin_angle) > EPS_F) {
				float coeff = angle / sin_angle;
				x *= coeff;
				y *= coeff;
				z *= coeff;
			}
		}
	}
	void make_exp() {
		float angle = std::sqrt(x*x + y*y + z*z);
		float sin_angle = std::sin(angle);
		w = std::cos(angle);
		if(sin_angle > EPS_F) {
			float coeff = sin_angle / angle;
			x *= coeff;
				y *= coeff;
				z *= coeff;
		}
	}

	/// Convert quaternion to equivalent euler angle rotation (XYZ)
	Vec3 to_euler() const {
		return unit().to_mat().to_euler();
	}

	/// Convert quaternion to equivalent rotation matrix (orthonormal, 3x3)
	Mat4 to_mat() const {
		return Mat4{
			Vec4{1-2*y*y-2*z*z, 2*x*y + 2*z*w, 2*x*z - 2*y*w, 0.0f},
			Vec4{2*x*y - 2*z*w, 1-2*x*x-2*z*z, 2*y*z + 2*x*w, 0.0f},
			Vec4{2*x*z + 2*y*w, 2*y*z - 2*x*w, 1-2*x*x-2*y*y, 0.0f},
			Vec4{0.0f, 0.0f, 0.0f, 1.0f}
		};
	}

	/// Apply rotation to given vector 
	Vec3 rotate(Vec3 v) const {
		return (((*this) * Quat(v, 0)) * conjugate()).complex();
	}

	/// Spherical linear interpolation between this and another quaternion weighted by t.
	Quat slerp(Quat q, float t) {
		float omega = std::acos(clamp(x*q.x + y*q.y + 
									  z*q.z + w*q.w, 
									  -1.0f, 1.0f));
		
		if(std::abs(omega) < 16.0f * FLT_EPSILON) {
			omega = 16.0f * FLT_EPSILON;
		}
		float som = std::sin(omega);
		float st0 = std::sin((1-t) * omega) / som;
		float st1 = std::sin(t * omega) / som;

		return Quat(x*st0 + q.x*st1,
					y*st0 + q.y*st1,
					z*st0 + q.z*st1,
					w*st0 + q.w*st1);
	}

	/// Are all members real numbers?
	bool valid() const {
		return !(std::isinf(x) || std::isinf(y) || std::isinf(z) || std::isinf(w));
	}

	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		float data[4] = {};
	};
};

inline Quat slerp(Quat q0, Quat q1, float t) {
	return q0.slerp(q1, t);
}

inline std::ostream& operator<<(std::ostream& out, Quat q) {
	out << "Quat{" << q.x << "," << q.y << "," << q.z << "," << q.w << "}";
	return out;
}
