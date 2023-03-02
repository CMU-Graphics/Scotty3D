
#include "test.h"
#include "geometry/halfedge.h"

#include <unordered_set>

static void expect_loop(Halfedge_Mesh& mesh, Halfedge_Mesh const &after) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();
    size_t numBoundaryFaces = 0;
    for (auto f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
        if (f->boundary) {
            numBoundaryFaces++;
        }
    }
	if (mesh.loop_subdivide()) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
		// check for expected number of elements
		if (numVerts + numEdges != mesh.vertices.size()) {
			throw Test::error("Loop subdivision didn't create the expected number of vertices!");
		}
		if (numEdges * 2 + (numFaces - numBoundaryFaces) * 3 != mesh.edges.size()) {
			throw Test::error("Loop subdivision didn't create the expected number of edges!");
		}
		if ((numFaces - numBoundaryFaces) * 4 != mesh.faces.size() - numBoundaryFaces) {
			throw Test::error("Loop subdivision didn't create the expected number of faces!");
		}

		for (auto f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
			if (f->degree() != 3 && !f->boundary) {
				throw Test::error("Loop subdivision created a non-triangular face!");
			}
		}

		// check mesh shape:
		if (auto diff = Test::differs(mesh, after)) {
			throw Test::error("Result does not match expected: " + diff.value());
		}

	} else {
		throw Test::error("Subdivide rejected!");
	}
}

/*
EDGE CASE

Loop subdivides a triangulated square
*/
Test test_a2_go1_loop_edge_square("a2.go1.loop.edge.square", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({	
		Vec3{-1.0f, 0.0f, -1.0f}, 	Vec3{-1.0f, 0.0f, 1.0f},
		Vec3{1.0f, 0.0f, -1.0f}, 	Vec3{1.0f, 0.0f, 1.0f}
	},{
		{0, 1, 2}, 
		{2, 1, 3}
	});

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({	
		Vec3{-0.75f, 0.0f, -0.75f}, Vec3{-0.75f, 0.0f, 0.75f},
		Vec3{0.75f, 0.0f, -0.75f}, 	Vec3{0.75f, 0.0f, 0.75f},
		Vec3{-1.0f, 0.0f, 0.0f}, 	Vec3{0.0f, 0.0f, 0.0f},
		Vec3{0.0f, 0.0f, -1.0f}, 	Vec3{0.0f, 0.0f, 1.0f},
		Vec3{1.0f, 0.0f, 0.0f}
	}, {
		{4, 5, 6},
		{5, 7, 8},
		{4, 1, 5},
		{5, 2, 6},
		{5, 1, 7},
		{6, 0, 4},
		{3, 8, 7},
		{8, 2, 5},
	});

	expect_loop(mesh, after);
});

/*
BASIC CASE

Loop subdivides a triangulated cube
*/
Test test_a2_go1_loop_basic_cube("a2.go1.loop.basic.cube", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({	
		Vec3{-0.5f, -0.5f, -0.5f}, Vec3{0.5f, -0.5f, -0.5f},
		Vec3{-0.5f, 0.5f, -0.5f}, Vec3{0.5f, 0.5f, -0.5f},
		Vec3{0.5f, -0.5f, 0.5f}, Vec3{0.5f, 0.5f, 0.5f},
		Vec3{-0.5f, -0.5f, 0.5f}, Vec3{-0.5f, 0.5f, 0.5f}
	},{
		{2, 0, 1},
		{3, 2, 1},
		{3, 1, 4},
		{5, 3, 4},
		{5, 4, 6},
		{7, 5, 6},
		{7, 6, 0},
		{2, 7, 0},
		{7, 2, 3},
		{5, 7, 3},
		{0, 6, 4},
		{1, 0, 4}
	});

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({	
		Vec3{-0.35f, -0.35f, -0.275f}, 		Vec3{0.3125f, -0.3125f, -0.40625f},
		Vec3{-0.3125f, 0.3125f, -0.40625f}, Vec3{0.35f, 0.35f, -0.275f},
		Vec3{0.35f, -0.35f, 0.275f}, 		Vec3{0.3125f, 0.3125f, 0.40625f},
		Vec3{-0.3125f, -0.3125f, 0.40625f}, Vec3{-0.35f, 0.35f, 0.275f},
		Vec3{0.0f, 0.0f, -0.5f}, 			Vec3{0.375f, 0.0f, -0.375f},
		Vec3{0.5f, 0.0f, 0.0f}, 			Vec3{0.375f, 0.0f, 0.375f},
		Vec3{0.0f, 0.0f, 0.5f}, 			Vec3{-0.375f, 0.0f, 0.375f},
		Vec3{-0.5f, 0.0f, 0.0f},			Vec3{-0.375f, 0.0f, -0.375f},
		Vec3{0.0f, 0.375f, -0.375f}, 		Vec3{-0.375f, 0.375f, -0.125f},
		Vec3{0.0f, 0.5f, 0.0f}, 			Vec3{0.375f, 0.375f, 0.125f},
		Vec3{0.0f, 0.375f, 0.375f}, 		Vec3{0.0f, -0.375f, 0.375f},
		Vec3{-0.375f, -0.375f, 0.125f}, 	Vec3{0.0f, -0.5f, 0.0f},
		Vec3{0.375f, -0.375f, -0.125f}, 	Vec3{0.0f, -0.375f, -0.375f}
	}, {
		{8, 1, 9},   {3, 16, 9},   {9, 24, 10},  {3, 9, 10},   {10, 4, 11},  {5, 19, 11},  {11, 21, 12}, {5, 11, 12},
		{12, 6, 13}, {7, 20, 13},  {13, 22, 14}, {7, 13, 14},  {14, 0, 15},  {8, 2, 15},   {0, 25, 15},  {2, 17, 15},
		{8, 9, 16},  {2, 8, 16},   {16, 18, 17}, {14, 15, 17}, {7, 14, 17},  {2, 16, 17},  {16, 3, 18},  {7, 17, 18},
		{18, 3, 19}, {10, 11, 19}, {3, 10, 19},  {5, 20, 19},  {18, 19, 20}, {12, 13, 20}, {5, 12, 20},  {7, 18, 20},
		{11, 4, 21}, {6, 12, 21},  {21, 23, 22}, {13, 6, 22},  {0, 14, 22},  {6, 21, 22},  {21, 4, 23},  {0, 22, 23},
		{23, 4, 24}, {9, 1, 24},   {4, 10, 24},  {1, 25, 24},  {23, 24, 25}, {8, 15, 25},  {1, 8, 25},   {0, 23, 25}
	});

	expect_loop(mesh, after);
});
