#include "test.h"
#include "geometry/halfedge.h"
#include <iostream>
#include <set>

static void expect_collapse(Halfedge_Mesh& mesh, Halfedge_Mesh::FaceRef face, Halfedge_Mesh const &after) {

	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();
	size_t faceDeg = face->degree();

	if (auto ret = mesh.collapse_face(face)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
		// check for expected number of elements
		if (numVerts - faceDeg + 1 != mesh.vertices.size()) {
			throw Test::error("Some vertices were not erased!");
		}
		if (numEdges <= mesh.edges.size()) {
			throw Test::error("Some edges were not erased!");
		}
		if (numFaces <= mesh.faces.size()) {
			throw Test::error("Collapse face did not erase a face!");
		}
		// check mesh shape:
        if (auto diff = Test::differs(mesh, after)) {
		    throw Test::error("Result does not match expected: " + diff.value());
	    }
	} else {
		throw Test::error("Collapse face rejected operation!");
	}
}

/*
BASIC CASE

Initial mesh:
23---24--- 4--- 5--- 6
 |    |    |    |    |
 |    |    |    |    |
 |    |    |    |    |
22---21--- 3--- 0--- 7
 |    |    |    |    |
 |    |    |    |    |
 |    |    |    |    |
20---16--- 2--- 1--- 8
 |    |    |    |    |
 |    |    |    |    |
 |    |    |    |    |
19---15---10--- 9---11
 |    |    |    |    |
 |    |    |    |    |
 |    |    |    |    |
18---17---14---13---12

Collapse Face on Face: 3-0-1-2
*/
Test test_a2_lx3_collapse_face_basic_planar("a2.lx3.collapse_face.basic.planar", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{0.25f, 0.0f, 0.25f},   Vec3{0.25f, 0.0f, 0.0f},
        Vec3{0.0f, 0.0f, 0.0f},     Vec3{0.0f, 0.0f, 0.25f},
        Vec3{0.0f, 0.0f, 0.5f},     Vec3{0.25f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, 0.5f},     Vec3{0.5f, 0.0f, 0.25f},
        Vec3{0.5f, 0.0f, 0.0f},     Vec3{0.25f, 0.0f, -0.25f},
        Vec3{0.0f, 0.0f, -0.25f},   Vec3{0.5f, 0.0f, -0.25f},
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.25f, 0.0f, -0.5f},
        Vec3{0.0f, 0.0f, -0.5f},    Vec3{-0.25f, 0.0f, -0.25f},
        Vec3{-0.25f, 0.0f, 0.0f},   Vec3{-0.25f, 0.0f, -0.5f},
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, -0.25f},
        Vec3{-0.5f, 0.0f, 0.0f},    Vec3{-0.25f, 0.0f, 0.25f},
        Vec3{-0.5f, 0.0f, 0.25f},   Vec3{-0.5f, 0.0f, 0.5f},
        Vec3{-0.25f, 0.0f, 0.5f}
	}, {
        {3, 0, 1, 2},
        {5, 0, 3, 4},
        {7, 0, 5, 6},
        {1, 0, 7, 8},
        {1, 9, 10, 2},
        {11, 9, 1, 8},
        {13, 9, 11, 12},
        {10, 9, 13, 14},
        {10, 15, 16, 2},
        {17, 15, 10, 14},
        {19, 15, 17, 18},
        {16, 15, 19, 20},
        {16, 21, 3, 2},
        {22, 21, 16, 20},
        {24, 21, 22, 23},
        {3, 21, 24, 4}
	});

    Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
        Vec3{0.125f, 0.0f, 0.125f}, Vec3{0.0f, 0.0f, 0.5f},
        Vec3{0.25f, 0.0f, 0.5f},    Vec3{0.5f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, 0.25f},    Vec3{0.5f, 0.0f, 0.0f},
        Vec3{0.25f, 0.0f, -0.25f},  Vec3{0.0f, 0.0f, -0.25f},
        Vec3{0.5f, 0.0f, -0.25f},   Vec3{0.5f, 0.0f, -0.5f},
        Vec3{0.25f, 0.0f, -0.5f},   Vec3{0.0f, 0.0f, -0.5f},
        Vec3{-0.25f, 0.0f, -0.25f}, Vec3{-0.25f, 0.0f, 0.0f},
        Vec3{-0.25f, 0.0f, -0.5f},  Vec3{-0.5f, 0.0f, -0.5f},
        Vec3{-0.5f, 0.0f, -0.25f},  Vec3{-0.5f, 0.0f, 0.0f},
        Vec3{-0.25f, 0.0f, 0.25f},  Vec3{-0.5f, 0.0f, 0.25f},
        Vec3{-0.5f, 0.0f, 0.5f},    Vec3{-0.25f, 0.0f, 0.5f}
	}, {
        {0, 1, 2},
        {4, 0, 2, 3},
        {0, 4, 5},
        {0, 6, 7},
        {8, 6, 0, 5},
        {10, 6, 8, 9},
        {7, 6, 10, 11},
        {7, 12, 13, 0},
        {14, 12, 7, 11},
        {16, 12, 14, 15},
        {13, 12, 16, 17},
        {0, 13, 18},
        {19, 18, 13, 17},
        {21, 18, 19, 20},
        {0, 18, 21, 1}
	});

	expect_collapse(mesh, face, after);
});

/*
EDGE CASE

Initial mesh:
2---3
|   |
|   |
|   |
0---1

Collapse Face on Face: BOUNDARY
*/
Test test_a2_lx3_collapse_face_edge_boundary("a2.lx3.collapse_face.edge.boundary", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{0, 0, 0}, Vec3{1, 0, 0}, 
        Vec3{0, 1, 0}, Vec3{1, 1, 0}
	}, {
        {0, 1, 3, 2}
	});

	Halfedge_Mesh::FaceRef face = mesh.faces.begin();
	std::advance(face, 1);

	//TODO: Hmmmmmm collapsing a boundary face should maybe be okay.
    //      Not sure what that would result in?
	if (mesh.collapse_face(face)) {
		throw Test::error("EDGE CASE: Did not reject collapsing a boundary face!");
	}
});
