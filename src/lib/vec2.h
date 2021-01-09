
#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>

#include "log.h"

struct Vec2 {

    Vec2() {
        x = 0.0f;
        y = 0.0f;
    }
    explicit Vec2(float _x, float _y) {
        x = _x;
        y = _y;
    }
    explicit Vec2(float f) {
        x = y = f;
    }
    explicit Vec2(int _x, int _y) {
        x = (float)_x;
        y = (float)_y;
    }

    Vec2(const Vec2&) = default;
    Vec2& operator=(const Vec2&) = default;
    ~Vec2() = default;

    float& operator[](int idx) {
        assert(idx >= 0 && idx <= 1);
        return data[idx];
    }
    float operator[](int idx) const {
        assert(idx >= 0 && idx <= 1);
        return data[idx];
    }

    Vec2 operator+=(Vec2 v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    Vec2 operator-=(Vec2 v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }
    Vec2 operator*=(Vec2 v) {
        x *= v.x;
        y *= v.y;
        return *this;
    }
    Vec2 operator/=(Vec2 v) {
        x /= v.x;
        y /= v.y;
        return *this;
    }

    Vec2 operator+=(float s) {
        x += s;
        y += s;
        return *this;
    }
    Vec2 operator-=(float s) {
        x -= s;
        y -= s;
        return *this;
    }
    Vec2 operator*=(float s) {
        x *= s;
        y *= s;
        return *this;
    }
    Vec2 operator/=(float s) {
        x /= s;
        y /= s;
        return *this;
    }

    Vec2 operator+(Vec2 v) const {
        return Vec2(x + v.x, y + v.y);
    }
    Vec2 operator-(Vec2 v) const {
        return Vec2(x - v.x, y - v.y);
    }
    Vec2 operator*(Vec2 v) const {
        return Vec2(x * v.x, y * v.y);
    }
    Vec2 operator/(Vec2 v) const {
        return Vec2(x / v.x, y / v.y);
    }

    Vec2 operator+(float s) const {
        return Vec2(x + s, y + s);
    }
    Vec2 operator-(float s) const {
        return Vec2(x - s, y - s);
    }
    Vec2 operator*(float s) const {
        return Vec2(x * s, y * s);
    }
    Vec2 operator/(float s) const {
        return Vec2(x / s, y / s);
    }

    bool operator==(Vec2 v) const {
        return x == v.x && y == v.y;
    }
    bool operator!=(Vec2 v) const {
        return x != v.x || y != v.y;
    }

    /// Absolute value
    Vec2 abs() const {
        return Vec2(std::abs(x), std::abs(y));
    }
    /// Negation
    Vec2 operator-() const {
        return Vec2(-x, -y);
    }
    /// Are all members real numbers?
    bool valid() const {
        return !(std::isinf(x) || std::isinf(y) || std::isnan(x) || std::isnan(y));
    }

    /// Modify vec to have unit length
    Vec2 normalize() {
        float n = norm();
        x /= n;
        y /= n;
        return *this;
    }
    /// Return unit length vec in the same direction
    Vec2 unit() const {
        float n = norm();
        return Vec2(x / n, y / n);
    }

    float norm_squared() const {
        return x * x + y * y;
    }
    float norm() const {
        return std::sqrt(norm_squared());
    }

    Vec2 range(float min, float max) const {
        if(!valid()) return Vec2();
        Vec2 r = *this;
        float range = max - min;
        while(r.x < min) r.x += range;
        while(r.x >= max) r.x -= range;
        while(r.y < min) r.y += range;
        while(r.y >= max) r.y -= range;
        return r;
    }

    union {
        struct {
            float x;
            float y;
        };
        float data[2] = {};
    };
};

inline Vec2 operator+(float s, Vec2 v) {
    return Vec2(v.x + s, v.y + s);
}
inline Vec2 operator-(float s, Vec2 v) {
    return Vec2(v.x - s, v.y - s);
}
inline Vec2 operator*(float s, Vec2 v) {
    return Vec2(v.x * s, v.y * s);
}
inline Vec2 operator/(float s, Vec2 v) {
    return Vec2(s / v.x, s / v.y);
}

/// Take minimum of each component
inline Vec2 hmin(Vec2 l, Vec2 r) {
    return Vec2(std::min(l.x, r.x), std::min(l.y, r.y));
}
/// Take maximum of each component
inline Vec2 hmax(Vec2 l, Vec2 r) {
    return Vec2(std::max(l.x, r.x), std::max(l.y, r.y));
}

/// 2D dot product
inline float dot(Vec2 l, Vec2 r) {
    return l.x * r.x + l.y * r.y;
}

inline std::ostream& operator<<(std::ostream& out, Vec2 v) {
    out << "{" << v.x << "," << v.y << "}";
    return out;
}
