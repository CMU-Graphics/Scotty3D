#include "test.h"
#include "geometry/halfedge.h"

static void expect_extrude(Halfedge_Mesh &mesh, Halfedge_Mesh::FaceRef face, Vec3 move, float shrink, Halfedge_Mesh const &after) {
	if (auto ret = mesh.extrude_face(face)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}

		// check if returned edge is the same edge
		if (ret != face) {
			throw Test::error("Did not return the same edge!");
		}
		mesh.extrude_positions(face, move, shrink);

		// check mesh shape:
		if (auto difference = Test::differs(mesh, after, Test::CheckAllBits)) {
			throw Test::error("Resulting mesh did not match expected: " + *difference);
		}
	} else {
		throw Test::error("extrude_face rejected operation!");
	}
}

/*
Initial mesh:
0---1\
|   | \
|   |  2
|   | /
3---4/

Extrude face on Face: 0-3-4-1

After mesh:
0-----1\
|\   /| \
| 2-3 |  \
| | | |   4
| 5-6 |  /
|/   \| /
7-----8/
*/

Test test_a2_l4_extrude_face_simple_shrink("a2.l4.extrude_face.simple.shrink", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f),
		                                            Vec3(2.0f, 0.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f), Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0,3,4,1}, {1,4,2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f),           Vec3(1.0f, 1.0f, 0.0f),
		      Vec3(-0.5f, 0.5f, 0.0f),  Vec3(0.5f, 0.5f, 0.0f),
		                                                                Vec3(2.0f, 0.0f, 0.0f),
		      Vec3(-0.5f,-0.5f, 0.0f),  Vec3(0.5f,-0.5f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f),           Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0,2,3,1}, {0,7,5,2}, {2,5,6,3}, {3,6,8,1}, {5,7,8,6}, {1,8,4}
	});

	expect_extrude(mesh, face, Vec3(0.0f), 0.5f, after);
});

/*
Initial mesh:
0---1\
|   | \
|   |  2
|   | /
3---4/

Extrude face on Face: 0-3-4-1

After mesh:
2-----3\
|\   /| \
| 0-1 |  \
| | | |   4
| 7-8 |  /
|/   \| /
5-----6/
*/

Test test_a2_l4_extrude_face_simple_expand("a2.l4.extrude_face.simple.expand", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f),
		                                            Vec3(2.0f, 0.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f), Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0,3,4,1}, {1,4,2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f),           Vec3(1.0f, 1.0f, 0.0f),
		      Vec3(-2.0f, 2.0f, 0.0f),  Vec3(2.0f, 2.0f, 0.0f),
		                                                                Vec3(2.0f, 0.0f, 0.0f),
		      Vec3(-2.0f,-2.0f, 0.0f),  Vec3(2.0f,-2.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f),           Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0,2,3,1}, {0,7,5,2}, {2,5,6,3}, {3,6,8,1}, {5,7,8,6}, {1,8,4}
	});

	expect_extrude(mesh, face, Vec3(0.0f), -1.0f, after);
});

/*
Initial mesh:
0---1\
|   | \
|   |  2
|   | /
3---4/

Extrude face on Face: 0-3-4-1

After mesh:
02---13\
|     | \
|     |  4
|     | /
57---68/
*/

Test test_a2_l4_extrude_face_simple_same("a2.l4.extrude_face.simple.same", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f),
		                                            Vec3(2.0f, 0.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f), Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0,3,4,1}, {1,4,2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f),           Vec3(1.0f, 1.0f, 0.0f),
		      Vec3(-1.0f, 1.0f, 1.0f),  Vec3(1.0f, 1.0f, 1.0f),
		                                                                Vec3(2.0f, 0.0f, 0.0f),
		      Vec3(-1.0f,-1.0f, 1.0f),  Vec3(1.0f,-1.0f, 1.0f),
		Vec3(-1.0f,-1.0f, 0.0f),           Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0,2,3,1}, {0,7,5,2}, {2,5,6,3}, {3,6,8,1}, {5,7,8,6}, {1,8,4}
	});

	expect_extrude(mesh, face, Vec3(0.0f, 0.0f, 1.0f), 0.0f, after);
});

