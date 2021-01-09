
#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>

#include "vec3.h"

struct Line {

    Line() {
    }
    /// Create line from point and unit direction
    explicit Line(Vec3 point, Vec3 dir) : point(point), dir(dir.unit()) {
    }

    Line(const Line&) = default;
    Line& operator=(const Line&) = default;
    ~Line() = default;

    /// Get point on line at time t
    Vec3 at(float t) const {
        return point + t * dir;
    }

    /// Get closest point on line to pt
    Vec3 closest(Vec3 pt) const {
        Vec3 p = pt - point;
        float t = dot(p, dir);
        return at(t);
    }

    /// Get closest point on line to another line.
    /// Returns false if the closest point is 'backward' relative to the line's direction
    bool closest(Line other, Vec3& pt) const {
        Vec3 p0 = point - other.point;
        float a = dot(dir, other.dir);
        float b = dot(dir, p0);
        float c = dot(other.dir, p0);
        float t0 = (a * c - b) / (1.0f - a * a);
        float t1 = (c - a * b) / (1.0f - a * a);
        pt = at(t0);
        return t1 >= 0.0f;
    }

    Vec3 point, dir;
};

inline std::ostream& operator<<(std::ostream& out, Line l) {
    out << "Line{" << l.point << "," << l.dir << "}";
    return out;
}
