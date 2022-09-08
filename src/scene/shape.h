
#pragma once

#include <memory>
#include <variant>

#include "../geometry/indexed.h"
#include "../lib/mathlib.h"
#include "../pathtracer/samplers.h"
#include "../pathtracer/trace.h"

namespace Shapes {

class Sphere {
public:
	Sphere(float r = 1.0f) : radius(r) {
	}

	static Vec2 uv(Vec3 dir);

	BBox bbox() const;
	PT::Trace hit(Ray ray) const;
	Vec3 sample(Vec3 from) const;
	float pdf(Ray ray, Mat4 pdf_T = Mat4::I, Mat4 pdf_iT = Mat4::I) const;

	Indexed_Mesh to_mesh() const;

	Samplers::Sphere::Uniform sampler;
	float radius;
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

	Vec3 sample(Vec3 from) const {
		return std::visit([&](auto& s) { return s.sample(from); }, shape);
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
};

bool operator!=(const Shapes::Sphere& a, const Shapes::Sphere& b);
bool operator!=(const Shape& a, const Shape& b);
