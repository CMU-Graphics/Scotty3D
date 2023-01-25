
#pragma once

#include "../lib/mathlib.h"
#include "../platform/gl.h"

#include "trace.h"

struct RNG;

namespace PT {

template<typename Primitive> class BVH {
public:
	class Node {
	public:
		BBox bbox;
		size_t start, size, l, r;

		// A node is a leaf if l == r, since all interior nodes must have distinct children
		bool is_leaf() const;
		friend class BVH<Primitive>;
	};

	BVH() = default;
	BVH(std::vector<Primitive>&& primitives, size_t max_leaf_size = 1);
	void build(std::vector<Primitive>&& primitives, size_t max_leaf_size = 1);

	BVH(BVH&& src) = default;
	BVH& operator=(BVH&& src) = default;

	BVH(const BVH& src) = delete;
	BVH& operator=(const BVH& src) = delete;

	BBox bbox() const;
	Trace hit(const Ray& ray) const;

	template<typename P = Primitive>
	typename std::enable_if<std::is_copy_assignable_v<P>, BVH<P>>::type copy() const;

	uint32_t visualize(GL::Lines& lines, GL::Lines& active, uint32_t level,
	                   const Mat4& trans) const;
	size_t n_primitives() const;

	std::vector<Primitive> destructure();
	void clear();

	Vec3 sample(RNG &rng, Vec3 from) const;
	float pdf(Ray ray, const Mat4& T = Mat4::I, const Mat4& iT = Mat4::I) const;

	std::vector<Primitive> primitives;
	std::vector<Node> nodes;
	size_t root_idx = 0;

private:
	size_t new_node(BBox box = {}, size_t start = 0, size_t size = 0, size_t l = 0, size_t r = 0);
};

} // namespace PT
