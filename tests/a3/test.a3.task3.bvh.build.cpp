#include "test.h"
#include "pathtracer/bvh.h"
#include "pathtracer/tri_mesh.h"

// Construct a triangle
static PT::Triangle add_triangle(std::vector<PT::Tri_Mesh_Vert>& verts, Vec3 v0, Vec3 v1, Vec3 v2, uint32_t i) {
	Vec3 norm = Vec3(0, 1, 0);
	verts.push_back({v0, norm});
	verts.push_back({v1, norm});
	verts.push_back({v2, norm});
	return PT::Triangle(verts.data(), i, i + 1, i + 2);
}

// Check invariants of a bvh
static void check_invariants(const PT::BVH<PT::Triangle>& bvh, const PT::BVH<PT::Triangle>::Node& node, size_t mls) {
	// If we are on a leaf node
	if (node.is_leaf()) {
		// Check number of primitives
		if (node.size > mls) {
			throw Test::error("A leaf contains more primitives than max_leaf_size!");
		}
		BBox prims;
		for (size_t i = node.start; i < node.start + node.size; i++) {
			prims.enclose(bvh.primitives[i].bbox());
		}
		// Check that the bbox is correct
		if (Test::differs(node.bbox.min, prims.min) || Test::differs(node.bbox.max, prims.max)) {
			throw Test::error("A leaf's bbox was not tight!");
		}
		return;
	}
	// Check sizes are correct
	if (node.size != bvh.nodes.at(node.l).size + bvh.nodes.at(node.r).size) {
		throw Test::error("A node's children contain a different number of primitives than the node!");
	}
	// Check that we don't have infinite recursion
	if (node.size == bvh.nodes.at(node.l).size || node.size == bvh.nodes.at(node.r).size) {
		throw Test::error("A node placed all primitives in one child!");
	}
	// Check that all the primitives below in one or the other nodes
	if (bvh.nodes.at(node.l).start + bvh.nodes.at(node.l).size != bvh.nodes.at(node.r).start) {
		throw Test::error("A node's right child primitives do not begin right after its left child primitives!");
	}
	// Check that the bbox's are correct
	if (Test::differs(node.bbox.min, hmin(bvh.nodes.at(node.l).bbox.min, bvh.nodes.at(node.r).bbox.min)) ||
	    Test::differs(node.bbox.max, hmax(bvh.nodes.at(node.l).bbox.max, bvh.nodes.at(node.r).bbox.max))) {
		throw Test::error("A node's bbox was not tight about its children!");
	}

	// check if node's bbox is combined bbox of all primitives it contains
	BBox prims;
	for (size_t i = node.start; i < node.start + node.size; i++) {
		prims.enclose(bvh.primitives.at(i).bbox());
	}
	if (Test::differs(node.bbox.min, prims.min) || Test::differs(node.bbox.max, prims.max)) {
		throw Test::error("A node's bbox was not tight about its primitives!");
	}

	// check left and right children recursively
	check_invariants(bvh, bvh.nodes.at(node.l), mls);
	check_invariants(bvh, bvh.nodes.at(node.r), mls);
}

static void expect_bvh(std::vector<Vec3>& verts, size_t max_leaf_size, float exp_overlap = 1.99f) {

	if (verts.size() % 3 != 0) {
		die("Input verts vector does not have a multiple of 3 size!");
	}

	std::vector<PT::Tri_Mesh_Vert> tri_verts;
	std::vector<PT::Triangle> prims, prims_cpy;

	tri_verts.reserve(verts.size());
	prims.reserve(verts.size() / 3);
	prims_cpy.reserve(verts.size() / 3);

	for (size_t i = 0; i < verts.size(); i += 3) {
		PT::Triangle t =
			add_triangle(tri_verts, verts.at(i), verts.at(i + 1), verts.at(i + 2), static_cast<uint32_t>(i));
		prims.push_back(t);
		prims_cpy.push_back(t);
	}

	PT::BVH<PT::Triangle> bvh;
	bvh.build(std::move(prims), max_leaf_size);

	// Check if all prims are present. This is O(n^2), but we only run this check on small inputs.
	// This could be replaced with a hash set, but we'd rather only compare the values up to epsilon,
	// and doing a bunch of quantization doesn't seem worthwhile.

	if (bvh.nodes.at(bvh.root_idx).size != bvh.primitives.size()) {
		throw Test::error("Root node does not include all primitives!");
	}
	for (size_t i = 0; i < bvh.primitives.size(); i++) {
		auto it = std::find(prims_cpy.begin(), prims_cpy.end(), bvh.primitives.at(i));
		if (it == prims_cpy.end()) {
			throw Test::error("Created a primitive not part of the input set??");
		}
		prims_cpy.erase(it);
	}
	if (!prims_cpy.empty()) {
		throw Test::error("Did not include all input primitives!");
	}

	// check if there is an excessive overlap between children bboxes
	// note that this only works on tests that check first level children
	float root_sa = bvh.nodes.at(bvh.root_idx).bbox.surface_area();
	float root_l_sa = bvh.nodes.at(bvh.nodes.at(bvh.root_idx).l).bbox.surface_area();
	float root_r_sa = bvh.nodes.at(bvh.nodes.at(bvh.root_idx).r).bbox.surface_area();
	if (root_l_sa + root_r_sa > exp_overlap * root_sa) {
		throw Test::error("Root partition is obviously suboptimal!");
	}

	// check quality of all nodes
	check_invariants(bvh, bvh.nodes.at(bvh.root_idx), max_leaf_size);
}

Test test_a3_task3_bvh_build_simple("a3.task3.bvh.build.simple", []() {
	auto verts = std::vector{
								Vec3{0, 0, 0}, Vec3{1, 0, 0}, Vec3{0, 1, 0}, 
								Vec3{0, 0, 1}, Vec3{1, 0, 1}, Vec3{0, 1, 1}, 
								Vec3{0, 0, 3}, Vec3{1, 0, 3}, Vec3{0, 1, 3}, 
								Vec3{0, 0, 4}, Vec3{1, 0, 4}, Vec3{0, 1, 4}, 
							};

	expect_bvh(verts, 2, 1);
	expect_bvh(verts, 4, 2);
});
