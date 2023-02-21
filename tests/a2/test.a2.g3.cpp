#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_cc(Halfedge_Mesh& mesh) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	// count the number of new edges to be generated after catmull-clark subdiv
	// count the number of quad faces to be generated after catmull-clark subdiv
	size_t ce = 0;
	size_t cf = 0;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		if (!f->boundary) {
			ce += f->degree();
			cf += f->degree() - 1;
		}
	}

	mesh.catmark_subdivide();

	if (auto msg = mesh.validate()) {
		throw Test::error("Invalid mesh: " + msg.value().second);
	}

	// check that all faces are degree 4
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		if (!f->boundary && f->degree() != 4) {
			throw Test::error("Catmull-clark subdivision created a non-quad face!");
		}
	}

	// check for expected number of elements
	if (numVerts + numEdges + numFaces - mesh.n_boundaries() != mesh.vertices.size()) {
		throw Test::error("Catmull-clark subdivision did not create the expected number of vertices!");
	}
	if (numEdges * 2 + ce != mesh.edges.size()) {
		throw Test::error("Catmull-clark subdivision did not create the expected number of edges!");
	}
	if (numFaces + cf != mesh.faces.size()) {
		throw Test::error("Catmull-clark subdivision did not create the expected number of faces!");
	}
}

// Catmull clark subdivides a square
Test test_a2_g3_catmull_clark_square("meshedit.a2.g3.catmull_clark.square", []() {
	Halfedge_Mesh sqr = Halfedge_Mesh::from_indexed_faces(
		{ Vec3{-1.0f, 1.0f, 0.0f}, Vec3{ 1.0f, 1.0f, 0.0f},
		  Vec3{-1.0f,-1.0f, 0.0f}, Vec3{ 1.0f,-1.0f, 0.0f} },
		{ {2,3,1,0} }
	);

	expect_cc(sqr);

	float const C = 0.75f;
	Halfedge_Mesh sqr_cc = Halfedge_Mesh::from_indexed_faces(
		{ Vec3{-C,    C,    0.0f}, Vec3{ 0.0f, 1.0f, 0.0f }, Vec3{ C,    C,    0.0f},
		  Vec3{-1.0f, 0.0f, 0.0f}, Vec3{ 0.0f, 0.0f, 0.0f }, Vec3{ 1.0f, 0.0f, 0.0f},
		  Vec3{-C,   -C,    0.0f}, Vec3{ 0.0f,-1.0f, 0.0f }, Vec3{ C,   -C,    0.0f} },
		{ {3,4,1,0}, {4,5,2,1},
		  {6,7,4,3}, {7,8,5,4} }
	);

	if (auto diff = Test::differs(sqr, sqr_cc)) {
		throw Test::error("Result does not match expected: " + diff.value());
	}
});

// Catmull clark subdivides a cube with square faces
Test test_a2_g3_catmull_clark_quad_cube("meshedit.a2.g3.catmull_clark.quad_cube", []() {
	Halfedge_Mesh quad_cube({{3, 0, 1, 2}, {5, 3, 2, 4}, {7, 5, 4, 6}, {0, 7, 6, 1}, {0, 3, 5, 7}, {6, 4, 2, 1}},
	                        {Vec3{-1.0f, 1.0f, 1.0f}, Vec3{-1.0f, 1.0f, -1.0f},
	                         Vec3{-1.0f, -1.0f, -1.0f}, Vec3{-1.0f, -1.0f, 1.0f},
	                         Vec3{1.0f, -1.0f, -1.0f}, Vec3{1.0f, -1.0f, 1.0f},
	                         Vec3{1.0f, 1.0f, -1.0f}, Vec3{1.0f, 1.0f, 1.0f}});

	Halfedge_Mesh quad_cube_cc(
		{{3, 0, 1, 2},     {5, 0, 3, 4},     {7, 0, 5, 6},     {1, 0, 7, 8},     {7, 9, 10, 8},    {11, 9, 7, 6},
	     {13, 9, 11, 12},  {10, 9, 13, 14},  {13, 15, 16, 14}, {17, 15, 13, 12}, {19, 15, 17, 18}, {16, 15, 19, 20},
	     {19, 21, 22, 20}, {23, 21, 19, 18}, {3, 21, 23, 4},   {22, 21, 3, 2},   {10, 24, 1, 8},   {16, 24, 10, 14},
	     {22, 24, 16, 20}, {1, 24, 22, 2},   {11, 25, 17, 12}, {5, 25, 11, 6},   {23, 25, 5, 4},   {17, 25, 23, 18}},
		{Vec3{-1.0f, 0.0f, 0.0f},
	     Vec3{-0.75f, 0.0f, 0.75f},
	     Vec3{-0.555556f, 0.555556f, 0.555556f},
	     Vec3{-0.75f, 0.75f, 0.0f},
	     Vec3{-0.555556f, 0.555556f, -0.555556f},
	     Vec3{-0.75f, 0.0f, -0.75f},
	     Vec3{-0.555556f, -0.555556f, -0.555556f},
	     Vec3{-0.75f, -0.75f, 0.0f},
	     Vec3{-0.555556f, -0.555556f, 0.555556f},
	     Vec3{0.0f, -1.0f, 0.0f},
	     Vec3{0.0f, -0.75f, 0.75f},
	     Vec3{0.0f, -0.75f, -0.75f},
	     Vec3{0.555556f, -0.555556f, -0.555556f},
	     Vec3{0.75f, -0.75f, 0.0f},
	     Vec3{0.555556f, -0.555556f, 0.555556f},
	     Vec3{1.0f, 0.0f, 0.0f},
	     Vec3{0.75f, 0.0f, 0.75f},
	     Vec3{0.75f, 0.0f, -0.75f},
	     Vec3{0.555556f, 0.555556f, -0.555556f},
	     Vec3{0.75f, 0.75f, 0.0f},
	     Vec3{0.555556f, 0.555556f, 0.555556f},
	     Vec3{0.0f, 1.0f, 0.0f},
	     Vec3{0.0f, 0.75f, 0.75f},
	     Vec3{0.0f, 0.75f, -0.75f},
	     Vec3{0.0f, 0.0f, 1.0f},
	     Vec3{0.0f, 0.0f, -1.0f}});

	expect_cc(quad_cube);

	if (auto diff = Test::differs(quad_cube, quad_cube_cc)) {
		throw Test::error("Result does not match expected: " + diff.value());
	}
});
