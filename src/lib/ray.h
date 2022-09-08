
#pragma once

#include <cmath>
#include <limits>
#include <ostream>

#include "../lib/mathlib.h"
#include "../lib/spectrum.h"

struct Ray {

	Ray() = default;

	/// Create Ray from point and direction
	explicit Ray(Vec3 point, Vec3 dir,
	             Vec2 dist_bounds = Vec2{0.0f, std::numeric_limits<float>::max()},
	             uint32_t depth = 0)
		: point(point), dir(dir.unit()), depth(depth), dist_bounds(dist_bounds) {
	}

	Ray(const Ray&) = default;
	Ray& operator=(const Ray&) = default;
	~Ray() = default;

	/// Get point on Ray at time t
	Vec3 at(float t) const {
		return point + t * dir;
	}

	/// Move ray into the space defined by this tranform matrix
	void transform(const Mat4& trans) {
		point = trans * point;
		dir = trans.rotate(dir);
		float d = dir.norm();
		dist_bounds *= d;
		dir /= d;
	}

	/// The origin or starting point of this ray
	Vec3 point;
	/// The direction the ray travels in
	Vec3 dir;
	/// Total attenuation new light will be scaled by to get to the source of this ray
	Spectrum throughput = Spectrum(1.0f);
	/// Recursive depth of ray
	uint32_t depth = 0;

	/// The minimum and maximum distance at which this ray can encounter collisions
	/// note that this field is mutable, meaning it can be changed on const Rays
	mutable Vec2 dist_bounds = Vec2(0.0f, std::numeric_limits<float>::max());
};

inline std::ostream& operator<<(std::ostream& out, Ray r) {
	out << "Ray{" << r.point << "," << r.dir << "}";
	return out;
}
