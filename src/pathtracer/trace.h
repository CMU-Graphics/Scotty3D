
#pragma once

#include "../lib/mathlib.h"

class Material;

namespace PT {

struct Trace {

	Trace() = default;

	/// Create Trace from point and direction
	explicit Trace(bool hit, Vec3 origin, Vec3 position, Vec3 normal, Vec2 uv)
		: hit(hit), distance((position - origin).norm()), position(position), normal(normal),
		  origin(origin), uv(uv) {
	}

	bool hit = false;

	float distance = 0.0f;
	Vec3 position, normal, origin;
	Vec2 uv;

	const Material* material = nullptr;

	static Trace min(const Trace& l, const Trace& r) {
		if (l.hit && r.hit) {
			if (l.distance < r.distance) return l;
			return r;
		}
		if (l.hit) return l;
		if (r.hit) return r;
		return {};
	}

	void transform(const Mat4& T, const Mat4& N) {
		position = T * position;
		origin = T * origin;
		normal = N.rotate(normal).unit();
		distance = (position - origin).norm();
	}
};

} // namespace PT
