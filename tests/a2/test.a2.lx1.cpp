#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_erase(Halfedge_Mesh& mesh, Halfedge_Mesh::VertexRef vertex, Halfedge_Mesh const &after) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	// collect all vertices belonging to faces touching v
	std::set<uint32_t> face_vert_ids;
	Halfedge_Mesh::HalfedgeRef he = vertex->halfedge;
	Halfedge_Mesh::HalfedgeRef heOrig = he;
	do {
		Halfedge_Mesh::HalfedgeRef tempOrig = he;
		do {
			face_vert_ids.insert(he->vertex->id);
			he = he->next;
		} while (he->next != tempOrig);
		he = he->twin;
	} while (he != heOrig);
	face_vert_ids.erase(vertex->id);

	size_t vertDeg = vertex->degree();
	uint32_t erased_id = vertex->id;

	if (auto ret = mesh.dissolve_vertex(vertex)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
		// check if input vertex is erased
		std::set<uint32_t> vert_ids;
		for (Halfedge_Mesh::VertexRef ve = mesh.vertices.begin(); ve != mesh.vertices.begin(); ve++) {
			vert_ids.insert(ve->id);
		}
		if (vert_ids.find(erased_id) != vert_ids.end()) {
			throw Test::error("Erase vertex did not erase the input vertex!");
		}
		// check if returned face contains vertices originally belonging to its one-ring
		std::set<uint32_t> new_face_vert_ids;
		Halfedge_Mesh::HalfedgeRef new_he = ret.value()->halfedge;
		Halfedge_Mesh::HalfedgeRef new_he_orig = new_he;
		do {
			new_face_vert_ids.insert(new_he->vertex->id);
			new_he = new_he->next;
		} while (new_he != new_he_orig);
		if (face_vert_ids != new_face_vert_ids) {
			throw Test::error("Erase vertex did not return a face with correct vertices!");
		}
		// check for expected number of elements
		if (numVerts - 1 != mesh.vertices.size()) {
			throw Test::error("Erase vertex did not erase a vertex!");
		}
		if (numEdges - vertDeg != mesh.edges.size()) {
			throw Test::error("Erase vertex did not erase an edge!");
		}
		if (numFaces - vertDeg + 1 != mesh.faces.size()) {
			throw Test::error("Erase vertex did not erase a face!");
		}
		// check mesh shape:
        if (auto diff = Test::differs(mesh, after)) {
		    throw Test::error("Result does not match expected: " + diff.value());
	    }
	} else {
		throw Test::error("Erase vertex rejected operation!");
	}
}

/*
BASIC CASE

Initial mesh:
1---3
|\ /|
| 4 |
|/ \|
0---2

Dissolve Vertex on Vertex: 4
*/
Test test_a2_lx1_dissolve_vertex_basic_tris("a2.lx1.dissolve_vertex.basic.tris", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f},
        Vec3{0.0f, 0.0f, 0.0f}
	}, {
		{3, 2, 4}, 
        {0, 1, 4}, 
        {2, 0, 4}, 
        {1, 3, 4}
	});
	
    Halfedge_Mesh::VertexRef vertex = mesh.vertices.begin();
	std::advance(vertex, 4);

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f}
	}, {
		{2, 0, 1, 3}
	});

	expect_erase(mesh, vertex, after);
});

/*
EDGE CASE

Initial mesh:
2---3
|   |
|   |
|   |
0---1

Dissolve Vertex on Vertex: 3
*/
Test test_a2_lx1_dissolve_vertex_edge_boundary("a2.lx1.dissolve_vertex.edge.boundary", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{0, 0, 0}, Vec3{1, 0, 0}, 
        Vec3{0, 1, 0}, Vec3{1, 1, 0}
	}, {
        {0, 1, 3, 2}
	});
	Halfedge_Mesh::VertexRef vertex = mesh.vertices.begin();

	if (mesh.dissolve_vertex(vertex)) {
		throw Test::error("EDGE CASE: Did not reject erasing a boundary vertex!");
	}
});
