
#pragma once

#include "../lib/mathlib.h"
#include "../scene/delta_light.h"
#include "../scene/shape.h"

#include <variant>

#include "trace.h"
#include "tri_mesh.h"

namespace PT {

class Instance {
public:
	Instance(Shape const * shape, Material* material, const Mat4& T)
		: T(T), iT(T.inverse()), material(material), geometry(shape) {
		has_transform = T != Mat4::I;
	}
	Instance(Tri_Mesh const * mesh, Material* material, const Mat4& T)
		: T(T), iT(T.inverse()), material(material), geometry(mesh) {
		has_transform = T != Mat4::I;
	}

	BBox bbox() const {
		auto box = std::visit([](const auto& g) { return g->bbox(); }, geometry);
		if (has_transform) box.transform(T);
		return box;
	}

	Trace hit(Ray ray) const {
		if (has_transform) ray.transform(iT);
		auto trace = std::visit([&](const auto& g) { return g->hit(ray); }, geometry);
		if (trace.hit) {
			trace.material = material;
			if (has_transform) trace.transform(T, iT.T());
		}
		return trace;
	}

	uint32_t visualize(GL::Lines& lines, GL::Lines& active, uint32_t level, Mat4 vtrans) const {
		if (has_transform) vtrans = vtrans * T;
		return std::visit(overloaded{[&](const Tri_Mesh* mesh) {
										 return mesh->visualize(lines, active, level, vtrans);
									 },
		                             [](const auto&) { return 0u; }},
		                  geometry);
	}

	Vec3 sample(RNG &rng, Vec3 from) const {
		if (has_transform) from = iT * from;
		auto dir = std::visit([&](const auto& g) { return g->sample(rng, from); }, geometry);
		if (has_transform) dir = T.rotate(dir).unit();
		return dir;
	}

	float pdf(Ray ray, Mat4 pdf_T = Mat4::I, Mat4 pdf_iT = Mat4::I) const {
		if (has_transform) {
			pdf_T = pdf_T * T;
			pdf_iT = iT * pdf_iT;
		}
		return std::visit([&](const auto& g) { return g->pdf(ray, pdf_T, pdf_iT); }, geometry);
	}

private:
	Mat4 T, iT;
	bool has_transform = false;

	const Material* material = nullptr;
	std::variant<const Shape*, const Tri_Mesh*> geometry;
};

class Light_Instance {
public:
	Light_Instance(Delta_Light* light, const Mat4& T) : T(T), iT(T.inverse()), light(light) {
		has_transform = T != Mat4::I;
	}

	Delta_Lights::Incoming incoming(Vec3 from) const {
		if (has_transform) from = iT * from;
		Delta_Lights::Incoming ret = light->incoming(from);
		if (has_transform) ret.transform(T);
		return ret;
	}

private:
	Mat4 T, iT;
	bool has_transform = false;
	const Delta_Light* light = nullptr;
};

} // namespace PT
