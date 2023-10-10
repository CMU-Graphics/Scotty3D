#include "test.h"
#include "geometry/util.h"
#include "pathtracer/aggregate.h"
#include "pathtracer/tri_mesh.h"

Test test_a3_task3_bvh_hit_empty("a3.task3.bvh.hit.empty", []() {
	PT::Aggregate scene;

	Ray ray(Vec3(), Vec3(0, 0, 1));

	PT::Trace ret = scene.hit(ray);
	PT::Trace exp(false, Vec3(), Vec3(), Vec3(), Vec2{});

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

Test test_a3_task3_bvh_hit_simple_triangle("a3.task3.bvh.hit.simple.triangle", []() {
	std::vector<Indexed_Mesh::Vert> verts;
	verts.push_back({Vec3(0, 0, 0), Vec3(1, 0, 0), Vec2(0, 0), 0});
	verts.push_back({Vec3(1, 0, 0), Vec3(1, 0, 0), Vec2(0, 0), 1});
	verts.push_back({Vec3(0, 1, 0), Vec3(1, 0, 0), Vec2(0, 0), 2});
	std::vector<Indexed_Mesh::Index> indices = {0, 1, 2};
	PT::Tri_Mesh mesh = PT::Tri_Mesh(Indexed_Mesh(std::move(verts), std::move(indices)), true);

	Ray ray(Vec3(0, 0, -1), Vec3(0, 0, 1));

	// Note the difference between this and triangle.hit
	PT::Trace ret = mesh.hit(ray);
	PT::Trace exp(true, Vec3(0, 0, -1), Vec3(0, 0, 0), Vec3(1, 0, 0),
	              Vec2{});

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

Test test_a3_task3_bvh_hit_simple_sphere("a3.task3.bvh.hit.simple.sphere", []() {
	PT::Tri_Mesh mesh = PT::Tri_Mesh(Util::closed_sphere_mesh(1.0f, 1), true);

	Ray ray(Vec3(0, 0, -2), Vec3(0, 0, 1));

	PT::Trace ret = mesh.hit(ray);
	PT::Trace exp(true, Vec3(0, 0, -2), Vec3(0, 0, -1), Vec3(0, 0, -1), Vec2{0.75f, 0.5f});

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});
