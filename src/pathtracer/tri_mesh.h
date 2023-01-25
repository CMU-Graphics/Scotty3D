
#pragma once

#include "../geometry/indexed.h"
#include "../lib/mathlib.h"

#include "bvh.h"
#include "list.h"
#include "trace.h"

namespace PT {

struct Tri_Mesh_Vert {
	Vec3 position;
	Vec3 normal;
	Vec2 uv;
};

class Triangle {
public:
	BBox bbox() const;
	Trace hit(const Ray& ray) const;

	uint32_t visualize(GL::Lines&, GL::Lines&, uint32_t, const Mat4&) const {
		return 0u;
	}

	//sample a vector pointing to the triangle from point 'from':
	Vec3 sample(RNG &rng, Vec3 from) const;
	float pdf(Ray ray, const Mat4& T, const Mat4& iT) const;

	Triangle(Tri_Mesh_Vert* verts, uint32_t v0, uint32_t v1, uint32_t v2);

	bool operator==(const Triangle& rhs) const;

private:
	uint32_t v0, v1, v2;
	Tri_Mesh_Vert* vertex_list;
	friend class Tri_Mesh;
};

static_assert(std::is_copy_assignable_v<Triangle>);

class Tri_Mesh {
public:
	Tri_Mesh() = default;
	// You can only build Tri_Mesh from an Indexed_Mesh:
	Tri_Mesh(const Indexed_Mesh& mesh, bool use_bvh);

	Tri_Mesh(Tri_Mesh&& src) = default;
	Tri_Mesh& operator=(Tri_Mesh&& src) = default;
	Tri_Mesh(const Tri_Mesh& src) = delete;
	Tri_Mesh& operator=(const Tri_Mesh& src) = delete;

	Tri_Mesh copy() const;

	BBox bbox() const;
	Trace hit(const Ray& ray) const;

	uint32_t visualize(GL::Lines& lines, GL::Lines& active, uint32_t level,
	                   const Mat4& trans) const;

	size_t n_triangles() const;

	//sample a vector pointing to the mesh from point 'from':
	Vec3 sample(RNG &rng, Vec3 from) const;
	float pdf(Ray ray, const Mat4& T, const Mat4& iT) const;

private:
	bool use_bvh = true;
	std::vector<Tri_Mesh_Vert> verts;
	BVH<Triangle> triangle_bvh;
	List<Triangle> triangle_list;
};

} // namespace PT
