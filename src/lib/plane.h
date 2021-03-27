
#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>

#include "line.h"
#include "vec4.h"

struct Plane {

    Plane() = default;

    /// Create plane from (a,b,c,d)
    explicit Plane(Vec4 p) : p(p) {
    }
    /// Create plane from point and unit normal
    explicit Plane(Vec3 point, Vec3 n) {
        p.x = n.x;
        p.y = n.y;
        p.z = n.z;
        p.w = dot(point, n.unit());
    }

    Plane(const Plane&) = default;
    Plane& operator=(const Plane&) = default;
    ~Plane() = default;

    /// Calculate intersection point between plane and line.
    /// Returns false if the hit point is 'backward' along the line relative to pt.dir
    bool hit(Line line, Vec3& pt) const {
        Vec3 n = p.xyz();
        float t = (p.w - dot(line.point, n)) / dot(line.dir, n);
        pt = line.at(t);
        return t >= 0.0f;
    }

    Vec4 p;
};

inline std::ostream& operator<<(std::ostream& out, Plane v) {
    out << "Plane" << v.p;
    return out;
}
