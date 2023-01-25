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


Test test_a2_l5_extrude_face_simple("a2.l5.extrude_face.simple", []() {
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
