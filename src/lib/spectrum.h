
#pragma once

#include "vec3.h"
#include <cmath>
#include <ostream>

#define GAMMA 2.1f

struct Spectrum {

    Spectrum() {
        r = 0.0f;
        g = 0.0f;
        b = 0.0f;
    }
    explicit Spectrum(float _r, float _g, float _b) {
        r = _r;
        g = _g;
        b = _b;
    }
    explicit Spectrum(float f) {
        r = g = b = f;
    }

    Spectrum(const Spectrum&) = default;
    Spectrum& operator=(const Spectrum&) = default;
    ~Spectrum() = default;

    Spectrum operator+=(Spectrum v) {
        r += v.r;
        g += v.g;
        b += v.b;
        return *this;
    }
    Spectrum operator*=(Spectrum v) {
        r *= v.r;
        g *= v.g;
        b *= v.b;
        return *this;
    }
    Spectrum operator*=(float s) {
        r *= s;
        g *= s;
        b *= s;
        return *this;
    }

    static Spectrum direction(Vec3 v) {
        v.normalize();
        Spectrum s(std::abs(v.x), std::abs(v.y), std::abs(v.z));
        s.make_linear();
        return s;
    }

    void make_srgb() {
        r = std::pow(r, 1.0f / GAMMA);
        g = std::pow(g, 1.0f / GAMMA);
        b = std::pow(b, 1.0f / GAMMA);
    }
    void make_linear() {
        r = std::pow(r, GAMMA);
        g = std::pow(g, GAMMA);
        b = std::pow(b, GAMMA);
    }

    Spectrum operator+(Spectrum v) const {
        return Spectrum(r + v.r, g + v.g, b + v.b);
    }
    Spectrum operator-(Spectrum v) const {
        return Spectrum(r - v.r, g - v.g, b - v.b);
    }
    Spectrum operator*(Spectrum v) const {
        return Spectrum(r * v.r, g * v.g, b * v.b);
    }

    Spectrum operator+(float s) const {
        return Spectrum(r + s, g + s, b + s);
    }
    Spectrum operator*(float s) const {
        return Spectrum(r * s, g * s, b * s);
    }
    Spectrum operator/(float s) const {
        return Spectrum(r / s, g / s, b / s);
    }

    bool operator==(Spectrum v) const {
        return r == v.r && g == v.g && b == v.b;
    }
    bool operator!=(Spectrum v) const {
        return r != v.r || g != v.g || b != v.b;
    }

    float luma() const {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }

    bool valid() const {
        return !(std::isinf(r) || std::isinf(g) || std::isinf(b) || std::isnan(r) ||
                 std::isnan(g) || std::isnan(b));
    }

    Vec3 to_vec() const {
        return Vec3(r, g, b);
    }

    union {
        struct {
            float r;
            float g;
            float b;
        };
        float data[3] = {};
    };
};

inline Spectrum operator+(float s, Spectrum v) {
    return Spectrum(v.r + s, v.g + s, v.b + s);
}
inline Spectrum operator*(float s, Spectrum v) {
    return Spectrum(v.r * s, v.g * s, v.b * s);
}

inline std::ostream& operator<<(std::ostream& out, Spectrum v) {
    out << "Spectrum{" << v.r << "," << v.g << "," << v.b << "}";
    return out;
}
