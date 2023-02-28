#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_extrude(Halfedge_Mesh &mesh, Halfedge_Mesh::FaceRef face, Vec3 move, float shrink, Halfedge_Mesh const &after) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	std::set<Vec3> old_verts;
	{
		Halfedge_Mesh::HalfedgeCRef h = face->halfedge;
		do {
			old_verts.insert(h->vertex->position);
			h = h->next;
		} while (h != face->halfedge);
	}


	std::set<Halfedge_Mesh::FaceCRef> old_faces;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		old_faces.insert(f);
	}

	if (auto ret = mesh.extrude_face(face)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}

		// check if returned face is the same face
		if (ret != face) {
			throw Test::error("Did not return the same face!");
		}
		// check if returned face is the same as input face
		if (face->id != ret.value()->id) {
			throw Test::error("Bevel face did not return the input face!");
		}

		// check if new side faces are quads
		std::set<Halfedge_Mesh::FaceCRef> new_faces;
		for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
			new_faces.insert(f);
		}
		std::set<Halfedge_Mesh::FaceCRef> diff;
		std::set_difference(new_faces.begin(), new_faces.end(), old_faces.begin(), old_faces.end(),
		                    std::inserter(diff, diff.end()));
		for (Halfedge_Mesh::FaceCRef face_diff : diff) {
			// assume the access is safe because ids are created from the new mesh directly
			if (face_diff->degree() != 4) {
				throw Test::error("Bevel face created incorrect side faces!");
			}
		}

		// check the number of new elements created
		size_t faceDeg = ret.value()->degree();
		if (numVerts + faceDeg != mesh.vertices.size()) {
			throw Test::error("Bevel face created incorrect number of vertices!");
		}
		if (numEdges + faceDeg * 2 != mesh.edges.size()) {
			throw Test::error("Bevel face created incorrect number of edges!");
		}
		if (numFaces + faceDeg != mesh.faces.size()) {
			throw Test::error("Bevel face created incorrect number of faces!");
		}

		// extract the newly created vertices, check their positions are identical to input vertices
		std::set<Vec3> new_verts;
		Halfedge_Mesh::VertexRef v = --mesh.vertices.end();
		for (size_t i = 0; i < old_verts.size(); i++) {
			new_verts.insert(v->position);
			v--;
		}
		if (old_verts != new_verts) {
			throw Test::error("Bevel face created vertices at incorrect positions!");
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
BASIC CASE

Initial mesh:
0---1\
|   | \
|   |  2
|   | /
3---4/

Extrude Face on Face: 0-3-4-1, shrink in half and no translation

After mesh:
0-----1\
|\   /| \
| 2-3 |  \
| | | |   4
| 5-6 |  /
|/   \| /
7-----8/
*/
Test test_a2_l4_extrude_face_basic_shrink("a2.l4.extrude_face.basic.shrink", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f),
		                                            Vec3(2.0f, 0.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f), Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0, 3, 4, 1}, 
		{1, 4, 2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f),           Vec3(1.0f, 1.0f, 0.0f),
		      Vec3(-0.5f, 0.5f, 0.0f),  Vec3(0.5f, 0.5f, 0.0f),
		                                                                Vec3(2.0f, 0.0f, 0.0f),
		      Vec3(-0.5f,-0.5f, 0.0f),  Vec3(0.5f,-0.5f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f),           Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0, 2, 3, 1}, 
		{0, 7, 5, 2}, 
		{2, 5, 6, 3}, 
		{3, 6, 8, 1}, 
		{5, 7, 8, 6}, 
		{1, 8, 4}
	});

	expect_extrude(mesh, face, Vec3(0.0f), 0.5f, after);
});

/*
BASIC CASE

Initial mesh:
0---1\
|   | \
|   |  2
|   | /
3---4/

Extrude Face on Face: 0-3-4-1, expand by two and no translation

After mesh:
2-----3
|\   /| 
| 0-1-|--\
| | | |   4
| 7-8-|--/
|/   \| 
5-----6
*/
Test test_a2_l4_extrude_face_basic_expand("a2.l4.extrude_face.basic.expand", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f),
		                                            Vec3(2.0f, 0.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f), Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0, 3, 4, 1}, 
		{1, 4, 2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f),           Vec3(1.0f, 1.0f, 0.0f),
		      Vec3(-2.0f, 2.0f, 0.0f),  Vec3(2.0f, 2.0f, 0.0f),
		                                                                Vec3(2.0f, 0.0f, 0.0f),
		      Vec3(-2.0f,-2.0f, 0.0f),  Vec3(2.0f,-2.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f),           Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0, 2, 3, 1}, 
		{0, 7, 5, 2}, 
		{2, 5, 6, 3},
		{3, 6, 8, 1}, 
		{5, 7, 8, 6}, 
		{1, 8, 4}
	});

	expect_extrude(mesh, face, Vec3(0.0f), -1.0f, after);
});

/*
BASIC CASE

Initial mesh:
0---1\
|   | \
|   |  2
|   | /
3---4/

Extrude Face on Face: 0-3-4-1, no shrink and translate in z direction

After mesh:
02---13\
|     | \
|     |  4
|     | /
57---68/
*/
Test test_a2_l4_extrude_face_basic_up("a2.l4.extrude_face.basic.up", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f),
		                                            Vec3(2.0f, 0.0f, 0.0f),
		Vec3(-1.0f,-1.0f, 0.0f), Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0, 3, 4, 1}, 
		{1, 4, 2}
	});
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.0f, 0.0f),           Vec3(1.0f, 1.0f, 0.0f),
		      Vec3(-1.0f, 1.0f, 1.0f),  Vec3(1.0f, 1.0f, 1.0f),
		                                                                Vec3(2.0f, 0.0f, 0.0f),
		      Vec3(-1.0f,-1.0f, 1.0f),  Vec3(1.0f,-1.0f, 1.0f),
		Vec3(-1.0f,-1.0f, 0.0f),           Vec3(1.0f, -1.0f, 0.0f)
	}, {
		{0, 2, 3, 1}, 
		{0, 7, 5, 2}, 
		{2, 5, 6, 3}, 
		{3, 6, 8, 1}, 
		{5, 7, 8, 6}, 
		{1, 8, 4}
	});

	expect_extrude(mesh, face, Vec3(0.0f, 0.0f, 1.0f), 0.0f, after);
});

