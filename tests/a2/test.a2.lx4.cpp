
#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_inset(Halfedge_Mesh& mesh, Halfedge_Mesh::FaceRef face, Halfedge_Mesh const &after) {

	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	std::set<uint32_t> vert_ids;
	for (Halfedge_Mesh::VertexRef v = mesh.vertices.begin(); v != mesh.vertices.end(); v++) {
		vert_ids.insert(v->id);
	}

	uint32_t face_d = face->degree();

	if (auto ret = mesh.inset_vertex(face)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
		// check if returned vertex is a new vertex
		if (vert_ids.find(face->id) != vert_ids.end()) {
			throw Test::error("Inset vertex did not return a new vertex!");
		}
		// check for expected number of elements
		if (numVerts + 1 != mesh.vertices.size()) {
			throw Test::error("Inset vertex should create one vertex!");
		}
		if (numEdges + face_d != mesh.edges.size()) {
			throw Test::error("Inset vertex didn't create the right number of new edges!");
		}
		if (numFaces + face_d - 1 != mesh.faces.size()) {
			throw Test::error("Inset vertex didn't create the right number of new faces!");
		}
		// check mesh shape:
        if (auto diff = Test::differs(mesh, after)) {
		    throw Test::error("Result does not match expected: " + diff.value());
	    }
	} else {
		throw Test::error("Inset vertex rejected operation!");
	}
}

/*
BASIC CASE

Initial mesh:
1---3
|\  |
| \ |
|  \|
0---2

Inset Vertex on Face: 3-2-1
*/
Test test_a2_lx4_inset_vertex_basic_tri("a2.lx4.inset_vertex.basic.tri", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
		Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f}
    }, {
        {2, 0, 1}, 
        {3, 2, 1}
    });

	Halfedge_Mesh::FaceRef face = mesh.faces.begin();
	std::advance(face, 1);

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
		Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f},
		Vec3{0.166667f, 0.0f, 0.166667f}
    },{
        {2, 0, 1}, 
        {1, 3, 4}, 
        {4, 3, 2}, 
        {4, 2, 1}
    });

	expect_inset(mesh, face, after);
});

/*
EDGE CASE

Initial mesh:
1---3
|   |
|   |
|   |
0---2

Inset Vertex on Face: BOUNDARY
*/
Test test_a2_lx4_inset_vertex_edge_boundary("a2.lx4.inset_vertex.edge.boundary", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{0, 0, 0}, Vec3{1, 0, 0}, 
        Vec3{0, 1, 0}, Vec3{1, 1, 0}
    }, {
        {0, 1, 3, 2}
    });
	Halfedge_Mesh::FaceRef face = mesh.faces.begin();
	std::advance(face, 1);

	if (mesh.inset_vertex(face)) {
		throw Test::error("EDGE CASE: Did not reject insetting a vertex in a boundary face!");
	}
});
