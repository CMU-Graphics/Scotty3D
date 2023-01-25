
#pragma once

#include <memory>
#include <variant>

#include "../geometry/indexed.h"
#include "../lib/mathlib.h"

#include "introspect.h"

struct RNG;

namespace Delta_Lights {

//
// Delta_Lights in Scotty3D are lights that come from exactly one
//  direction or location. (so don't need to be sampled)
//
// the incoming(p) function describes the light incoming to
//  point p from the light

class Incoming {
public:
	Spectrum radiance; //radiance along ray from the light
	Vec3 direction; //direction to the light
	float distance; //distance to the light

	void transform(const Mat4& T) {
		direction = T.rotate(direction);
	}
};

class Point {
public:
	Incoming incoming(Vec3 p) const;

	Spectrum color = Spectrum(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;

	Spectrum display() const;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("color", t.color);
		f("intensity", t.intensity);
	}
	static inline const char *TYPE = "Point"; //used by introspect_variant<>
};

class Directional {
public:
	Incoming incoming(Vec3 p) const;

	Spectrum color = Spectrum(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;

	Spectrum display() const;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("color", t.color);
		f("intensity", t.intensity);
	}
	static inline const char *TYPE = "Directional"; //used by introspect_variant<>
};

class Spot {
public:
	Incoming incoming(Vec3 p) const;

	Spectrum color = Spectrum(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;
	float inner_angle = 30.0f; // falloff starts at this angle (degrees)
	float outer_angle = 45.0f; // falloff ends at this angle (degrees)

	GL::Lines to_gl() const;
	Spectrum display() const;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("color", t.color);
		f("intensity", t.intensity);
		f("inner_angle", t.inner_angle);
		f("outer_angle", t.outer_angle);
	}
	static inline const char *TYPE = "Spot"; //used by introspect_variant<>
};

} // namespace Delta_Lights

class Delta_Light {
public:
	Delta_Lights::Incoming incoming(Vec3 p) const {
		return std::visit([&](auto& l) { return l.incoming(p); }, light);
	}

	Spectrum display() const {
		return std::visit([&](auto& l) { return l.display(); }, light);
	}

	template<typename T> bool is() const {
		return std::holds_alternative<T>(light);
	}

	std::variant<Delta_Lights::Point, Delta_Lights::Directional, Delta_Lights::Spot> light;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		introspect_variant< I >(std::forward< F >(f), t.light);
	}
	static inline const char *TYPE = "Delta_Light";
};

bool operator!=(const Delta_Lights::Point& a, const Delta_Lights::Point& b);
bool operator!=(const Delta_Lights::Directional& a, const Delta_Lights::Directional& b);
bool operator!=(const Delta_Lights::Spot& a, const Delta_Lights::Spot& b);
bool operator!=(const Delta_Light& a, const Delta_Light& b);
