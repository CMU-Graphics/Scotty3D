#pragma once

#include "vec3.h"
#include <cmath>
#include <ostream>

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
	explicit Spectrum(Vec3 c) {
		r = c.x;
		g = c.y;
		b = c.z;
	}

	Spectrum(const Spectrum&) = default;
	Spectrum& operator=(const Spectrum&) = default;
	~Spectrum() = default;

	float& operator[](uint32_t idx) {
		assert(idx <= 2);
		return data[idx];
	}
	float operator[](uint32_t idx) const {
		assert(idx <= 2);
		return data[idx];
	}

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
		Spectrum s(0.5f * v.x + 0.5f, 0.5f * v.y + 0.5f, 0.5f * v.z + 0.5f);

		//HACK: adjust so that HDR_Image::tonemap_to(, e) will result in linear output:
		constexpr float e = 1.0f; //in case we change the default exposure
		//tonemap_to does 's.to_srgb()' last. So invert that:
		s = s.to_linear();
		//tonemap_to does r = 1.0f - std::exp(-r * e), invert that:
		s.r = std::log(std::max(0.001f, 1.0f - s.r)) / -e;
		s.g = std::log(std::max(0.001f, 1.0f - s.g)) / -e;
		s.b = std::log(std::max(0.001f, 1.0f - s.b)) / -e;

		return s;
	}

	static float to_linear(float f) {
		if (f > 0.04045f) {
			return std::pow((f + 0.055f) / 1.055f, 2.4f);
		} else {
			return f / 12.92f;
		}
	}
	static float to_srgb(float f) {
		if (f > 0.0031308f) {
			return 1.055f * (std::pow(f, (1.0f / 2.4f))) - 0.055f;
		} else {
			return f * 12.92f;
		}
	}

	Spectrum to_srgb() const {
		Spectrum ret;
		ret.r = to_srgb(r);
		ret.g = to_srgb(g);
		ret.b = to_srgb(b);
		return ret;
	}
	Spectrum to_linear() const {
		Spectrum ret;
		ret.r = to_linear(r);
		ret.g = to_linear(g);
		ret.b = to_linear(b);
		return ret;
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
		return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
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

inline std::string to_string(Spectrum const &v) {
	return "[" + std::to_string(v.r) + ", " + std::to_string(v.g) + ", " + std::to_string(v.b) + "]";
}
