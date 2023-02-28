#include "test.h"
#include "geometry/halfedge.h"

#include <map>
#include <set>

static void expect_triangulate(Halfedge_Mesh& mesh) {
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	// count the number of triangle edges/faces to be generated after triangulation
	size_t c = 0;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		if (!f->boundary && f->degree() > 3) c += f->degree() - 3;
	}

	std::map<uint32_t, Vec3> verts;
	for (Halfedge_Mesh::VertexRef v = mesh.vertices.begin(); v != mesh.vertices.end(); v++) {
		verts.insert({v->id, v->position});
	}
	std::set<uint32_t> edge_ids;
	for (Halfedge_Mesh::EdgeRef e = mesh.edges.begin(); e != mesh.edges.end(); e++) {
		edge_ids.insert(e->id);
	}
	std::set<uint32_t> face_ids;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		face_ids.insert(f->id);
	}

	//TODO: halfedges + twin relationships should all be preserved
	//TODO: shape of boundary faces should be preserved

	mesh.triangulate();

	if (auto msg = mesh.validate()) {
		throw Test::error("Invalid mesh: " + msg.value().second);
	}

	// check that all faces are degree 3
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		if (!f->boundary && f->degree() != 3) {
			throw Test::error("Triangulation created a non-triangular face!");
		}
	}

	// check for expected number of elements
	if (numEdges + c != mesh.edges.size()) {
		throw Test::error("Triangulation did not create the expected number of edges!");
	}
	if (numFaces + c != mesh.faces.size()) {
		throw Test::error("Triangulation did not create the expected number of faces!");
	}

	// check that original elements are preserved
	std::map<uint32_t, Vec3> new_verts;
	for (Halfedge_Mesh::VertexRef v = mesh.vertices.begin(); v != mesh.vertices.end(); v++) {
		new_verts.insert({v->id, v->position});
	}
	std::set<uint32_t> new_edge_ids;
	for (Halfedge_Mesh::EdgeRef e = mesh.edges.begin(); e != mesh.edges.end(); e++) {
		new_edge_ids.insert(e->id);
	}
	std::set<uint32_t> new_face_ids;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		new_face_ids.insert(f->id);
	}

	if (verts != new_verts) {
		throw Test::error("Triangulation should preserve original vertices!");
	}
	std::set<uint32_t> diff;
	std::set_difference(edge_ids.begin(), edge_ids.end(), new_edge_ids.begin(), new_edge_ids.end(),
	                    std::inserter(diff, diff.end()));
	if (diff.size() != 0) {
		throw Test::error("Triangulation should preserve original edges!");
	}
	std::set_difference(face_ids.begin(), face_ids.end(), new_face_ids.begin(), new_face_ids.end(),
	                    std::inserter(diff, diff.end()));
	if (diff.size() != 0) {
		throw Test::error("Triangulation should preserve original faces!");
	}
}


/*
BASIC CASE

Triangulates a square
*/
Test test_a2_g1_triangulate_basic_square("a2.g1.triangulate.basic.square", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3{-0.5f, 0.0f,-0.5f}, Vec3{-0.5f, 0.0f, 0.5f},
		Vec3{ 0.5f, 0.0f,-0.5f}, Vec3{ 0.5f, 0.0f, 0.5f}
	},{
		{1, 3, 2, 0}
	});

	// Many different implementations of triangulating, so just checks that all the faces are triangles and some other misc things
	expect_triangulate(mesh);
});

/*
BASIC CASE

Triangulates a cube with square faces
*/
Test test_a2_g1_triangulate_basic_quad_cube("a2.g1.triangulate.basic.quad_cube", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3{-1.0f, 1.0f, 1.0f}, 	Vec3{-1.0f, 1.0f, -1.0f},
		Vec3{-1.0f, -1.0f, -1.0f}, 	Vec3{-1.0f, -1.0f, 1.0f},
		Vec3{1.0f, -1.0f, -1.0f}, 	Vec3{1.0f, -1.0f, 1.0f},
		Vec3{1.0f, 1.0f, -1.0f}, 	Vec3{1.0f, 1.0f, 1.0f}
	}, {
		{3, 0, 1, 2}, 
		{5, 3, 2, 4}, 
		{7, 5, 4, 6}, 
		{0, 7, 6, 1},
		{0, 3, 5, 7}, 
		{6, 4, 2, 1}
	});

	// Many different implementations of triangulating, so just checks that all the faces are triangles and some other misc things
	expect_triangulate(mesh);
});
