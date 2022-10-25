
#pragma once

#include "../lib/mathlib.h"
#include "../scene/material.h"
#include "../scene/shape.h"

#include <memory>
#include <variant>

#include "trace.h"

#include "bvh.h"
#include "instance.h"
#include "list.h"
#include "tri_mesh.h"

namespace PT {

class Aggregate {
public:
	Aggregate(List<Instance>&& list) : underlying(std::move(list)) {
	}
	Aggregate(BVH<Instance>&& bvh) : underlying(std::move(bvh)) {
	}
	Aggregate(List<Aggregate>&& list) : underlying(std::move(list)) {
	}
	Aggregate(BVH<Aggregate>&& bvh) : underlying(std::move(bvh)) {
	}

	Aggregate() = default;
	Aggregate(const Aggregate& src) = delete;
	Aggregate& operator=(const Aggregate& src) = delete;
	Aggregate& operator=(Aggregate&& src) = default;
	Aggregate(Aggregate&& src) = default;

	BBox bbox() const {
		return std::visit([](const auto& o) { return o.bbox(); }, underlying);
	}

	Trace hit(Ray ray) const {
		return std::visit([&](const auto& o) { return o.hit(ray); }, underlying);
	}

	uint32_t visualize(GL::Lines& lines, GL::Lines& active, uint32_t level, Mat4 vtrans) const {
		return std::visit(overloaded{[&](const BVH<Aggregate>& bvh) {
										 return bvh.visualize(lines, active, level, vtrans);
									 },
		                             [&](const BVH<Instance>& bvh) {
										 return bvh.visualize(lines, active, level, vtrans);
									 },
		                             [](const auto&) { return 0u; }},
		                  underlying);
	}

	Vec3 sample(RNG &rng, Vec3 from) const {
		return std::visit([&](const auto& o) { return o.sample(rng, from); }, underlying);
	}

	float pdf(Ray ray, Mat4 T = Mat4::I, Mat4 iT = Mat4::I) const {
		return std::visit([&](const auto& o) { return o.pdf(ray, T, iT); }, underlying);
	}

private:
	std::variant<BVH<Instance>, List<Instance>, BVH<Aggregate>, List<Aggregate>> underlying;
};

} // namespace PT
