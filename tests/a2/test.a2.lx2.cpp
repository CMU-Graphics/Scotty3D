#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_erase(Halfedge_Mesh& mesh, Halfedge_Mesh::EdgeRef edge, Halfedge_Mesh const &after) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	std::set<uint32_t> vert_ids;
	Halfedge_Mesh::HalfedgeRef he = edge->halfedge;
	Halfedge_Mesh::HalfedgeRef heOrig = he;
	do {
		vert_ids.insert(he->vertex->id);
		he = he->next;
	} while (he != heOrig);
	he = heOrig->twin;
	heOrig = heOrig->twin;
	do {
		vert_ids.insert(he->vertex->id);
		he = he->next;
	} while (he != heOrig);

	uint32_t erased_id = edge->id;

	if (auto ret = mesh.dissolve_edge(edge)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
		// check if input edge is erased
		std::set<uint32_t> edge_ids;
		for (Halfedge_Mesh::EdgeRef ed = mesh.edges.begin(); ed != mesh.edges.end(); ed++) {
			edge_ids.insert(ed->id);
		}
		if (edge_ids.find(erased_id) != edge_ids.end()) {
			throw Test::error("Erase edge did not erase the input edge!");
		}
		// check if returned face contains vertices originally belonging to two faces touching the edge
		std::set<uint32_t> new_vert_ids;
		Halfedge_Mesh::HalfedgeRef new_he = ret.value()->halfedge;
		Halfedge_Mesh::HalfedgeRef new_he_orig = new_he;
		do {
			new_vert_ids.insert(new_he->vertex->id);
			new_he = new_he->next;
		} while (new_he != new_he_orig);
		if (vert_ids != new_vert_ids) {
			throw Test::error("Erase edge did not return a face with correct vertices!");
		}
		// check for expected number of elements
		if (numVerts != mesh.vertices.size()) {
			throw Test::error("Erase edge should not create/delete a vertex!");
		}
		if (numEdges - 1 != mesh.edges.size()) {
			throw Test::error("Erase edge did not erase an edge!");
		}
		if (numFaces - 1 != mesh.faces.size()) {
			throw Test::error("Erase edge did not erase a face!");
		}
		// check mesh shape:
        if (auto diff = Test::differs(mesh, after)) {
		    throw Test::error("Result does not match expected: " + diff.value());
	    }
	} else {
		throw Test::error("Erase edge rejected operation!");
	}
}

/*
BASIC CASE

Initial mesh:
2---3
|\  |
| \ |
|  \|
0---1

Dissolve Edge on Edge: 1-2
*/
Test test_a2_lx2_dissolve_edge_basic_tri_tri("a2.lx2.dissolve_edge.basic.tri_tri", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{0, 0, 0}, Vec3{1, 0, 0}, 
        Vec3{0, 1, 0}, Vec3{1, 1, 0}
	}, {
		{0, 1, 2}, 
        {2, 1, 3}
	});
	
    Halfedge_Mesh::EdgeRef edge = mesh.halfedges.begin()->next->edge;

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
        Vec3{0, 0, 0}, Vec3{1, 0, 0}, 
        Vec3{0, 1, 0}, Vec3{1, 1, 0}
	}, {
		{0, 1, 3, 2}
	});

	expect_erase(mesh, edge, after);
});

/*
EDGE CASE

Initial mesh:
2---3
|   |
|   |
|   |
0---1

Dissolve Edge on Edge: 0-2
*/
Test test_a2_lx2_dissolve_edge_edge_boundary("a2.lx2.dissolve_edge.edge.boundary", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{0, 0, 0}, Vec3{1, 0, 0}, 
        Vec3{0, 1, 0}, Vec3{1, 1, 0}
	}, {
        {0, 1, 3, 2}
	});

	Halfedge_Mesh::EdgeRef edge = mesh.halfedges.begin()->twin->next->edge;

	if (mesh.dissolve_edge(edge)) {
		throw Test::error("EDGE CASE: Did not reject erasing a boundary edge!");
	}
});
