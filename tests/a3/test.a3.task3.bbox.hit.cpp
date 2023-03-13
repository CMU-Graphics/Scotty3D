#include "test.h"
#include "lib/mathlib.h"

// Function to test if a ray we have intersects a bbox constructed from enclosing a vector of vertices
static bool try_intersect(std::vector<Vec3>& verts, Ray ray, Vec2& times) {
	BBox bbox;
	for (size_t i = 0; i < verts.size(); i++) {
		bbox.enclose(verts.at(i));
	}
	return bbox.hit(ray, times);
}

Test test_a3_task3_bbox_hit_simple("a3.task3.bbox.hit.simple", []() {
	// bbox with lower left corner Vec3(0,0,0) and top right corner Vec3(1,1,0)
	std::vector<Vec3> verts;
	verts.push_back(Vec3(0, 0, 0));
	verts.push_back(Vec3(1, 1, 0));

	Ray ray = Ray(Vec3(0.5f, 0.5f, -1.0f), Vec3(0, 0, 1));
	Vec2 dist_bounds = Vec2(0.f, FLT_MAX);

	if (!try_intersect(verts, ray, dist_bounds)) {
		throw Test::error("BBox did not detect any hits when it should!");
	}
});

Test test_a3_task3_bbox_hit_simple_miss("a3.task3.bbox.hit.simple_miss", []() {
	// bbox with lower left corner Vec3(0,0,0) and top right corner Vec3(1,1,0)
	std::vector<Vec3> verts;
	verts.push_back(Vec3(0, 0, 0));
	verts.push_back(Vec3(1, 1, 0));

	Ray ray = Ray(Vec3(-0.5f, -0.5f, -1.0f), Vec3(0, 0, 1));
	Vec2 dist_bounds = Vec2(0.f, FLT_MAX);

	if (try_intersect(verts, ray, dist_bounds)) {
		throw Test::error("BBox detected hits when it shouldn't have!");
	}
});

Test test_a3_task3_bbox_hit_simple_dist_bounds("a3.task3.bbox.hit.simple_dist_bounds", []() {
	// bbox with lower left corner Vec3(0,0,0) and top right corner Vec3(1,1,0)
	std::vector<Vec3> verts;
	verts.push_back(Vec3(0, 0, 0));
	verts.push_back(Vec3(1, 1, 0));

	Ray ray = Ray(Vec3(0.5f, 0.5f, -1.0f), Vec3(0, 0, 1));
	Vec2 dist_bounds = Vec2(2.f, FLT_MAX);

	if (try_intersect(verts, ray, dist_bounds)) {
		throw Test::error("BBox detected hits when it shouldn't have because of the dist_bounds!");
	}
});
