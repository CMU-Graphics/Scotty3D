
#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>

#include "log.h"

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
    explicit Vec3(int _x, int _y, int _z) {
        x = (float)_x;
        y = (float)_y;
        z = (float)_z;
    }
    explicit Vec3(float f) {
        x = y = z = f;
    }

    Vec3(const Vec3&) = default;
    Vec3& operator=(const Vec3&) = default;
    ~Vec3() = default;

    float& operator[](int idx) {
        assert(idx >= 0 && idx <= 2);
        return data[idx];
    }
    float operator[](int idx) const {
        assert(idx >= 0 && idx <= 2);
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
        return !(std::isinf(x) || std::isinf(y) || std::isinf(z) || std::isnan(x) ||
                 std::isnan(y) || std::isnan(z));
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

    /// Make sure all components are in the range [min,max) with floating point mod logic
    Vec3 range(float min, float max) const {
        if(!valid()) return Vec3();
        Vec3 r = *this;
        float range = max - min;
        while(r.x < min) r.x += range;
        while(r.x >= max) r.x -= range;
        while(r.y < min) r.y += range;
        while(r.y >= max) r.y -= range;
        while(r.z < min) r.z += range;
        while(r.z >= max) r.z -= range;
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

inline std::ostream& operator<<(std::ostream& out, Vec3 v) {
    out << "{" << v.x << "," << v.y << "," << v.z << "}";
    return out;
}

inline bool operator<(Vec3 l, Vec3 r) {
    if(l.x == r.x) {
        if(l.y == r.y) {
            return l.z < r.z;
        }
        return l.y < r.y;
    }
    return l.x < r.x;
}
