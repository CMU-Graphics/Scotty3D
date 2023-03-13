#include "test.h"
#include "pathtracer/trace.h"
#include "scene/shape.h"

#include <iostream>

// Function to test if a ray we have intersects a sphere with radius radius
static PT::Trace try_intersect(float radius, Ray ray) {
	return Shapes::Sphere{radius}.hit(ray);
}

Test test_a3_task2_sphere_hit_simple("a3.task2.sphere.hit.simple", []() {
	// Construct a ray starting at Vec3(0, 0, -2) in the direction of Vec3(0, 0, 1)
	Ray ray = Ray(Vec3(0, 0, -2), Vec3(0, 0, 1));

	// Expects we hit at Vec3(0, 0, -1) with a normal of Vec3(0, 0, -1) and uv corresponding to the uv function
	PT::Trace exp = PT::Trace(true, Vec3(0, 0, -2), Vec3(0, 0, -1),
	                          Vec3(0, 0, -1), Vec2{0.75f, 0.5f});

	PT::Trace ret = try_intersect(1, ray);

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

Test test_a3_task2_sphere_hit_simple_radius("a3.task2.sphere.hit.simple.radius", []() {
	// Construct a ray starting at Vec3(0, 0, -4) in the direction of Vec3(0, 0, 1)
	Ray ray = Ray(Vec3(0, 0, -4), Vec3(0, 0, 1));

	// Expects we hit at Vec3(0, 0, -2) with a normal of Vec3(0, 0, -1) and uv corresponding to the uv function
	PT::Trace exp = PT::Trace(true, Vec3(0, 0, -4), Vec3(0, 0, -2),
	                          Vec3(0, 0, -1), Vec2{0.75f, 0.5f});

	PT::Trace ret = try_intersect(2, ray);

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

Test test_a3_task2_sphere_hit_simple_second("a3.task2.sphere.hit.simple.second", []() {
	// Construct a ray starting at Vec3(0, 0, 0) in the direction of Vec3(0, 0, 1)
	Ray ray = Ray(Vec3(0, 0, 0), Vec3(0, 0, 1));

	// Expects we hit at Vec3(0, 0, 1) with a normal of Vec3(0, 0, 1) and uv corresponding to the uv function
	PT::Trace exp = PT::Trace(true, Vec3(0, 0, 0), Vec3(0, 0, 1),
	                          Vec3(0, 0, 1), Vec2{0.25f, 0.5f});

	PT::Trace ret = try_intersect(1, ray);

	if (auto diff = Test::differs(ret, exp)) {
		throw Test::error("Trace does not match expected: " + diff.value());
	}
});

