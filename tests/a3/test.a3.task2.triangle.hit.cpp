#include "test.h"
#include "pathtracer/trace.h"
#include "pathtracer/tri_mesh.h"

// Function to test if a ray we have intersects a triangle
static PT::Trace try_intersect(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 n0, Vec3 n1, Vec3 n2, Ray ray, Vec2 uv0 = {},
                               Vec2 uv1 = {}, Vec2 uv2 = {}) {
	std::vector<PT::Tri_Mesh_Vert> verts;
	verts.push_back({v0, n0, uv0});
	verts.push_back({v1, n1, uv1});
	verts.push_back({v2, n2, uv2});
	PT::Triangle tri = PT::Triangle(verts.data(), 0, 1, 2);

	return tri.hit(ray);
}

Test test_a3_task2_triangle_hit_simple("a3.task2.triangle.hit.simple", []() {
	// Construct a ray starting at Vec3(0, 0, -1) in the direction of Vec3(0, 0, 1)
	Ray ray = Ray(Vec3(0, 0, -1), Vec3(0, 0, 1));

	// Expects we hit at Vec3(0, 0, 0) with a normal of Vec3(1, 0, 0) and no uv
	PT::Trace exp = PT::Trace(true, Vec3(0, 0, -1), Vec3(0, 0, 0), Vec3(1, 0, 0), Vec2{});

	PT::Trace ret =
		try_intersect(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(1, 0, 0), Vec3(1, 0, 0), Vec3(1, 0, 0), ray);

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

Test test_a3_task2_triangle_hit_simple_normal("a3.task2.triangle.hit.simple.normal", []() {
	// Construct a ray starting at Vec3(0, 0, -1) in the direction of Vec3(0, 0, 1)
	Ray ray = Ray(Vec3(0, 0, -1), Vec3(0, 0, 1));

	// Expects we hit at Vec3(0, 0, 0) with a normal associated with first vertex and no uv
	PT::Trace exp = PT::Trace(true, Vec3(0, 0, -1), Vec3(0, 0, 0), Vec3(0.57735f, 0.57735f, 0.57735f), Vec2{});

	PT::Trace ret =
		try_intersect(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0.57735f, 0.57735f, 0.57735f), Vec3(0, 1, 0), Vec3(0, 0, 1), ray);

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

Test test_a3_task2_triangle_hit_simple_uv("a3.task2.triangle.hit.simple.uv", []() {
	// Construct a ray starting at Vec3(0, 0, -1) in the direction of Vec3(0, 0, 1)
	Ray ray = Ray(Vec3(0, 0, -1), Vec3(0, 0, 1));

	// Expects we hit at Vec3(0, 0, 0) with a normal of Vec3(1, 0, 0) and with uv associated with first vertex
	PT::Trace exp = PT::Trace(true, Vec3(0, 0, -1), Vec3(0, 0, 0), Vec3(1, 0, 0), Vec2(0.5f, 0.5f));

	PT::Trace ret =
		try_intersect(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(1, 0, 0), Vec3(1, 0, 0), Vec3(1, 0, 0), ray, Vec2(0.5f, 0.5f), Vec2(1.0f, 0.0f), Vec2(0.0f, 1.0f));

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});
