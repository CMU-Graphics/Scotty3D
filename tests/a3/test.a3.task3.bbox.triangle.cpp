#include "test.h"
#include "pathtracer/tri_mesh.h"

// Construct a triangle
static PT::Triangle add_triangle(std::vector<PT::Tri_Mesh_Vert>& verts, Vec3 v0, Vec3 v1, Vec3 v2) {
	Vec3 norm = Vec3(0, 1, 0);
	verts.push_back({v0, norm});
	verts.push_back({v1, norm});
	verts.push_back({v2, norm});
	return PT::Triangle(verts.data(), 0, 1, 2);
}

// Check the bbox from a triangle
static void check_bbox(Vec3 v0, Vec3 v1, Vec3 v2) {

	std::vector<PT::Tri_Mesh_Vert> verts;
	BBox bbox = add_triangle(verts, v0, v1, v2).bbox();

	Vec3 minVert = hmin(hmin(v0, v1), v2);
	Vec3 maxVert = hmax(hmax(v0, v1), v2);

	// This should account for making bbox nonzero volume by adding a small epsilon
	if (Test::differs(bbox.min, minVert)) {
		throw Test::error("Bbox does not have the correct minimum corner!");
	}

	if (Test::differs(bbox.max, maxVert)) {
		throw Test::error("Bbox does not have the correct maximum corner!");
	}
}

Test test_a3_task3_bbox_triangle_simple("a3.task3.bbox.triangle.simple", []() {
	check_bbox(Vec3(-1, -1, -1), Vec3(0.0f, 0.5f, 0.0f), Vec3(1, 1, 1));
});

Test test_a3_task3_bbox_triangle_simple_flat("a3.task3.bbox.triangle.simple.flat", []() {
	check_bbox(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(1, 0, 0));
});
