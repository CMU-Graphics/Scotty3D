#include "test.h"
#include "geometry/halfedge.h"

static void expect_make_boundary(Halfedge_Mesh &mesh, Halfedge_Mesh::FaceRef face, Halfedge_Mesh const &after) {
	if (auto ret = mesh.make_boundary(face)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
        Halfedge_Mesh::FaceRef f = ret.value();
		// check if returned face is the same face
		if (f != face) {
			throw Test::error("Did not return same face!");
		}
        // check that the returned face is a boundary face
        if (!f->boundary) {
            throw Test::error("Did not make the returned face a non-boundary face!");
        }
		// check mesh shape:
		if (auto difference = Test::differs(mesh, after, Test::CheckAllBits)) {
			throw Test::error("Resulting mesh did not match expected: " + *difference);
		}
	} else {
		throw Test::error("Make_boundary rejected operation!");
	}
}

/*
BASIC CASE

Initial mesh:
0---1--2\
|   |  | \
|   |  |  3
|   |  | /
4---5--6/

Make Boundary on Face: 1-5-6-2

After mesh:
0---1 2\
|   | | \
|   | |  3
|   | | /
4---5 6/
*/
Test test_a2_lx7_make_boundary_basic_inner("a2.lx7.make_boundary.basic.inner", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.0f, 1.0f, 0.0f), Vec3(1.1f, 1.0f, 0.0f),
		                                                         Vec3(2.2f, 0.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.1f,-1.0f, 0.0f), Vec3(1.4f, -1.0f, 0.0f)
	}, {
		{0, 4, 5, 1}, 
        {2, 6, 3}, 
        {1, 5, 6, 2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();
    std::advance(face, 2);

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.0f, 1.0f, 0.0f), Vec3(1.1f, 1.0f, 0.0f),
		                                                         Vec3(2.2f, 0.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.1f,-1.0f, 0.0f), Vec3(1.4f, -1.0f, 0.0f)
	}, {
		{0, 4, 5, 1}, 
		{2, 6, 3}
	});

	expect_make_boundary(mesh, face, after);
});

/*
BASIC CASE

Initial mesh:
0---1\     
|   | \
|   |  2
|   | /
3---4/

Make Boundary on Face: 1-4-2

After mesh:
0---1     
|   |
|   |
|   |
2---3
*/
Test test_a2_lx7_make_boundary_basic_tri("a2.lx7.make_boundary.basic.tri", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.05f, 1.0f, 0.0f),
		                                            Vec3(2.2f, 0.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.25f, -1.0f, 0.0f)
	}, {
		{0, 3, 4, 1}, 
		{1, 4, 2}
	});

	Halfedge_Mesh::FaceRef face = mesh.faces.begin();
    std::advance(face, 1);

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.05f, 1.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.25f, -1.0f, 0.0f)
	}, {
		{0, 2, 3, 1}, 
	});

	expect_make_boundary(mesh, face, after);
});
