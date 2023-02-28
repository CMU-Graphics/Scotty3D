#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_bevel_edge(Halfedge_Mesh& mesh, Halfedge_Mesh::EdgeRef edge, Vec3 dir, float dist, Halfedge_Mesh const &after) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	std::set<Vec3> old_verts;
	old_verts.insert(edge->halfedge->vertex->position);
	old_verts.insert(edge->halfedge->twin->vertex->position);

	std::set<uint32_t> face_ids;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		face_ids.insert(f->id);
	}

	uint32_t vertDeg =
		edge->halfedge->vertex->degree() + edge->halfedge->twin->vertex->degree();

	// create a copy of vertex IDs incident to edge to be bevelled
	Halfedge_Mesh::HalfedgeRef he = edge->halfedge;
	Halfedge_Mesh::HalfedgeRef heOrig = he;
	std::set<uint32_t> old_vert_connect;
	do {
		old_vert_connect.insert(he->twin->vertex->id);
		he = he->twin->next;
	} while (he != heOrig);
	he = he->twin;
	heOrig = he;
	do {
		old_vert_connect.insert(he->twin->vertex->id);
		he = he->twin->next;
	} while (he != heOrig);
	old_vert_connect.erase(edge->halfedge->vertex->id);
	old_vert_connect.erase(edge->halfedge->twin->vertex->id);

	if (auto ret = mesh.bevel_edge(edge)) {
        Halfedge_Mesh::FaceRef face = ret.value();
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}

		// check if returned face is a new face
		if (face_ids.find(face->id) != face_ids.end()) {
			throw Test::error("Bevel edge did not return a new face!");
		}

		// check the number of new elements created
		if (numVerts + vertDeg - 4 != mesh.vertices.size()) {
			throw Test::error("Bevel edge created incorrect number of vertices!");
		}
		if (numEdges + vertDeg - 3 != mesh.edges.size()) {
			throw Test::error("Bevel edge created incorrect number of edges!");
		}
		if (numFaces + 1 != mesh.faces.size()) {
			throw Test::error("Bevel edge created incorrect number of faces!");
		}

		// extract the newly created vertices, check their positions are identical to input vertex's
		std::set<Vec3> new_verts;
		std::set<Halfedge_Mesh::VertexRef> new_vert_ref;
		Halfedge_Mesh::HalfedgeRef face_he = face->halfedge;
		Halfedge_Mesh::HalfedgeRef face_he_orig = face_he;
        std::vector<Vec3> start_positions;
		do {
            start_positions.push_back(face_he->vertex->position);
			new_verts.insert(face_he->vertex->position);
			new_vert_ref.insert(face_he->vertex);
			face_he = face_he->next;
		} while (face_he != face_he_orig);

		if (old_verts != new_verts) {
			throw Test::error("Bevel edge created vertices at incorrect positions!");
		}
		
        mesh.bevel_positions(face, start_positions, dir, dist);

        // check mesh shape:
		if (auto difference = Test::differs(mesh, after, Test::CheckAllBits)) {
			throw Test::error("Resulting mesh did not match expected: " + *difference);
		}
	} else {
		throw Test::error("Bevel edge rejected operation!");
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

Bevel Edge on Edge: 1-2

After mesh:
  5-3
 / \|
1   6
|\ /
0-2
*/
Test test_a2_lx6_bevel_edge_basic_tri_tri("a2.lx6.bevel_edge.basic.tri_tri", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f}
	}, {
        {2, 0, 1}, 
        {3, 2, 1}
	});

	Halfedge_Mesh::EdgeRef edge = mesh.halfedges.begin()->next->next->edge;

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f}, Vec3{-0.5f, 0.0f, 0.25f}, 
        Vec3{0.25f, 0.0f, -0.5f}, Vec3{0.5f, 0.0f, 0.5f}, 
        Vec3{0.5f, 0.0f, -0.25f}, Vec3{-0.25f ,0.0f, 0.5f}
	}, {
        {2, 0, 1},
        {4, 5, 3},
        {2, 1, 5, 4}
	});

	expect_bevel_edge(mesh, edge, Vec3(0.0f, 1.0f, 0.0f), 0.25, after);
});

/*
EDGE CASE

Initial mesh:
1---3
|\  |
| \ |
|  \|
0---2

Bevel Edge on Edge: 0-2
*/
Test test_a2_lx6_bevel_edge_edge_tri_tri("a2.lx6.bevel_edge.edge.tri_tri", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f}
	}, {
        {2, 0, 1}, 
        {3, 2, 1}
	});

	Halfedge_Mesh::EdgeRef edge = mesh.halfedges.begin()->edge;

    if (mesh.bevel_edge(edge)) {
		throw Test::error("EDGE CASE: Did not reject bevelling a boundary edge!");
	}
});
