#include "test.h"
#include "geometry/indexed.h"
#include "pathtracer/tri_mesh.h"
#include "util/rand.h"

using PT::Tri_Mesh;
using PT::Tri_Mesh_Vert;
using PT::Triangle;

constexpr float tri_scale = 1.0f;
constexpr float translate_scale = 10.0f;

static Tri_Mesh random_mesh(RNG& gen, uint32_t n_tris) {

	std::vector<Indexed_Mesh::Vert> verts(n_tris * 3);
	std::vector<Indexed_Mesh::Index> inds(n_tris * 3);

	auto v = [&]() {
		return Vec3{gen.unit(), gen.unit(), gen.unit()} * tri_scale;
	};
	auto t = [&]() {
		return Vec3{gen.unit(), gen.unit(), gen.unit()} * translate_scale;
	};

	for (uint32_t i = 0; i < n_tris; i++) {
		Vec3 o = t();
		Vec3 v0 = v() + o;
		Vec3 v1 = v() + o;
		Vec3 v2 = v() + o;
		verts[i * 3 + 0] = Indexed_Mesh::Vert{v0, Vec3{0, 1, 0}, Vec2{}, 0};
		verts[i * 3 + 1] = Indexed_Mesh::Vert{v1, Vec3{0, 1, 0}, Vec2{}, 0};
		verts[i * 3 + 2] = Indexed_Mesh::Vert{v2, Vec3{0, 1, 0}, Vec2{}, 0};
		inds[i * 3 + 0] = i * 3 + 0;
		inds[i * 3 + 1] = i * 3 + 1;
		inds[i * 3 + 2] = i * 3 + 2;
	}

	return Tri_Mesh(Indexed_Mesh(std::move(verts), std::move(inds)), true);
}

static Ray random_ray(RNG& gen) {
	// Yes this doesn't sample directions uniformly; we don't want this to rely on the sphere sampler.
	return Ray{Vec3{gen.unit(), gen.unit(), gen.unit()} * translate_scale, Vec3{gen.unit(), gen.unit(), gen.unit()}};
}

Test test_a3_task3_bvh_fuzz("a3.task3.bvh.fuzz", []() {
	// Just build a bunch of large random BVHs and intersect random rays with them.

	RNG gen(462);
	constexpr uint32_t trials = 50;
	constexpr uint32_t rays = 500;
	constexpr uint32_t triangles = 5000;

	for (uint32_t i = 0; i < trials; i++) {
		uint32_t n = static_cast<uint32_t>(triangles * (gen.unit() + 0.5f));
		Tri_Mesh mesh = random_mesh(gen, n);
		for (uint32_t j = 0; j < rays; j++) {
			Ray ray = random_ray(gen);
			mesh.hit(ray);
		}
	}
});
