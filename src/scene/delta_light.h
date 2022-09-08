
#pragma once

#include <memory>
#include <variant>

#include "../geometry/indexed.h"
#include "../lib/mathlib.h"

namespace Delta_Lights {

class Sample {
public:
	Spectrum radiance;
	Vec3 direction;
	float distance;

	void transform(const Mat4& T) {
		direction = T.rotate(direction);
	}
};

class Point {
public:
	Sample sample(Vec3 p) const;

	Spectrum color = Spectrum(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;

	Spectrum display() const;
};

class Directional {
public:
	Sample sample(Vec3 p) const;

	Spectrum color = Spectrum(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;

	Spectrum display() const;
};

class Spot {
public:
	Sample sample(Vec3 p) const;

	Spectrum color = Spectrum(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;
	float inner_angle = 30.0f; // falloff starts at this angle (degrees)
	float outer_angle = 45.0f; // falloff ends at this angle (degrees)

	GL::Lines to_gl() const;
	Spectrum display() const;
};

} // namespace Delta_Lights

class Delta_Light {
public:
	Delta_Lights::Sample sample(Vec3 p) const {
		return std::visit([&](auto& l) { return l.sample(p); }, light);
	}

	Spectrum display() const {
		return std::visit([&](auto& l) { return l.display(); }, light);
	}

	template<typename T> bool is() const {
		return std::holds_alternative<T>(light);
	}

	std::variant<Delta_Lights::Point, Delta_Lights::Directional, Delta_Lights::Spot> light;
};

bool operator!=(const Delta_Lights::Point& a, const Delta_Lights::Point& b);
bool operator!=(const Delta_Lights::Directional& a, const Delta_Lights::Directional& b);
bool operator!=(const Delta_Lights::Spot& a, const Delta_Lights::Spot& b);
bool operator!=(const Delta_Light& a, const Delta_Light& b);
