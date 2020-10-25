
#pragma once

#include <cmath>
#include <limits>
#include <ostream>

#include "../lib/mathlib.h"

struct Ray {

    Ray() : time_bounds(0.0f, std::numeric_limits<float>::max()) {}

    /// Create Ray from point and direction
    explicit Ray(Vec3 point, Vec3 dir)
        : point(point), dir(dir.unit()), time_bounds(0.0f, std::numeric_limits<float>::max()) {}

    Ray(const Ray &) = default;
    Ray &operator=(const Ray &) = default;
    ~Ray() = default;

    /// Get point on Ray at time t
    Vec3 at(float t) const { return point + t * dir; }

    /// Move ray into the space defined by this tranform matrix
    void transform(const Mat4 &trans) {
        point = trans * point;
        dir = trans.rotate(dir);
    }

    /// The origin or starting point of this ray
    Vec3 point;
    /// The direction the ray travels in
    Vec3 dir;
    /// The minimum and maximum time/distance at which this ray should exist
    Vec2 time_bounds;
    /// Recursive depth of ray
    size_t depth = 0;
};

inline std::ostream &operator<<(std::ostream &out, Ray r) {
    out << "Ray{" << r.point << "," << r.dir << "}";
    return out;
}
