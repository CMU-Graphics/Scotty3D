
#pragma once

#include "../lib/mathlib.h"

namespace PT {

struct Trace {
	
	bool hit = false;
	float time = 0.0f;
	Vec3 position, normal;
	int material = 0;

	static Trace min(const Trace& l, const Trace& r) {
        if(l.hit && r.hit) {
            if(l.time < r.time) return l;
            return r;
        }
        if(l.hit) return l;
        if(r.hit) return r;
        return {};
    }

	void transform(const Mat4& transform, const Mat4& norm) {
        position = transform * position;
        normal = norm.rotate(normal);
    }
};

}
