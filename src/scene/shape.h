
#pragma once

#include <memory>
#include <variant>

#include "../geometry/indexed.h"
#include "../lib/mathlib.h"
#include "../pathtracer/samplers.h"
#include "../pathtracer/trace.h"

#include "introspect.h"

namespace Shapes {

class Sphere {
public:
	Sphere(float r = 1.0f) : radius(r) {
	}

	static Vec2 uv(Vec3 dir); //u = longitude, v = latitude, (0,1,0) is north pole, maps to (*,1). u increases cw starting at +x

	BBox bbox() const;
	PT::Trace hit(Ray ray) const;
	Vec3 sample(RNG &rng, Vec3 from) const;
	float pdf(Ray ray, Mat4 pdf_T = Mat4::I, Mat4 pdf_iT = Mat4::I) const;

	Indexed_Mesh to_mesh() const;

	Samplers::Sphere::Uniform sampler;
	float radius;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("radius", t.radius);
	}
	static inline const char *TYPE = "Sphere"; //used by introspect_variant<>
};

} // namespace Shapes

class Shape {
public:
	Shape(std::variant<Shapes::Sphere> shape) : shape(shape) {
	}
	Shape() : shape(Shapes::Sphere{}) {
	}

	BBox bbox() const {
		return std::visit([&](auto& s) { return s.bbox(); }, shape);
	}

	PT::Trace hit(Ray ray) const {
		return std::visit([&](auto& s) { return s.hit(ray); }, shape);
	}

	Vec3 sample(RNG &rng, Vec3 from) const {
		return std::visit([&](auto& s) { return s.sample(rng, from); }, shape);
	}

	float pdf(Ray ray, Mat4 pdf_T = Mat4::I, Mat4 pdf_iT = Mat4::I) const {
		return std::visit([&](auto& s) { return s.pdf(ray, pdf_T, pdf_iT); }, shape);
	}

	Indexed_Mesh to_mesh() const {
		return std::visit([&](auto& s) { return s.to_mesh(); }, shape);
	}

	template<typename T> bool is() const {
		return std::holds_alternative<T>(shape);
	}

	std::variant<Shapes::Sphere> shape;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		introspect_variant< I >(std::forward< F >(f), t.shape);
	}
	static inline const char *TYPE = "Shape";
};

bool operator!=(const Shapes::Sphere& a, const Shapes::Sphere& b);
bool operator!=(const Shape& a, const Shape& b);
