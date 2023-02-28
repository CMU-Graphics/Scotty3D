#include "test.h"
#include "geometry/halfedge.h"

#include <set>

static void expect_bevel_vertex(Halfedge_Mesh& mesh, Halfedge_Mesh::VertexRef vertex, Vec3 dir, float dist, Halfedge_Mesh const &after) {
	size_t numVerts = mesh.vertices.size();
	size_t numEdges = mesh.edges.size();
	size_t numFaces = mesh.faces.size();

	std::set<Vec3> old_verts;
	old_verts.insert(vertex->position);

	std::set<uint32_t> face_ids;
	for (Halfedge_Mesh::FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		face_ids.insert(f->id);
	}

	size_t vertDeg = vertex->degree();

	// create a copy of vertice IDs incident to vertex to be bevelled
	Halfedge_Mesh::HalfedgeRef he = vertex->halfedge;
	Halfedge_Mesh::HalfedgeRef heOrig = he;
	std::set<uint32_t> old_vert_connect;
	do {
		old_vert_connect.insert(he->twin->vertex->id);
		he = he->twin->next;
	} while (he != heOrig);

	if (auto ret = mesh.bevel_vertex(vertex)) {
		Halfedge_Mesh::FaceRef face = ret.value();
        if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}

		// check if returned face is a new face
		if (face_ids.find(face->id) != face_ids.end()) {
			throw Test::error("Bevel vertex did not return a new face!");
		}

		// check the number of new elements created
		if (numVerts + vertDeg - 1 != mesh.vertices.size()) {
			throw Test::error("Bevel vertex created incorrect number of vertices!");
		}
		if (numEdges + vertDeg != mesh.edges.size()) {
			throw Test::error("Bevel vertex created incorrect number of edges!");
		}
		if (numFaces + 1 != mesh.faces.size()) {
			throw Test::error("Bevel vertex created incorrect number of faces!");
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
			throw Test::error("Bevel vertex created vertices at incorrect positions!");
		}

		mesh.bevel_positions(face, start_positions, dir, dist);

		// check mesh shape:
		if (auto difference = Test::differs(mesh, after, Test::CheckAllBits)) {
			throw Test::error("Resulting mesh did not match expected: " + *difference);
		}
	} else {
		throw Test::error("Bevel vertex rejected operation!");
	}
}


/*
BASIC CASE

Initial mesh: Non-triangulated cube

Bevel Vertex on Vertex: 7 - Vec3{1.0f, 1.0f, 1.0f}, move in direction of corner normal for a distance of 0.5
*/
Test test_a2_lx5_bevel_vertex_basic_cube("a2.lx5.bevel_vertex.basic.cube", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{-1.0f, 1.0f, 1.0f},    Vec3{-1.0f, 1.0f, -1.0f},
        Vec3{-1.0f, -1.0f, -1.0f},  Vec3{-1.0f, -1.0f, 1.0f},
        Vec3{1.0f, -1.0f, -1.0f},   Vec3{1.0f, -1.0f, 1.0f},
        Vec3{1.0f, 1.0f, -1.0f},    Vec3{1.0f, 1.0f, 1.0f}
	}, {
        {3, 0, 1, 2}, 
        {5, 3, 2, 4}, 
        {7, 5, 4, 6}, 
        {0, 7, 6, 1}, 
        {0, 3, 5, 7}, 
        {6, 4, 2, 1}
	});

	Halfedge_Mesh::VertexRef vertex = mesh.vertices.begin();
	std::advance(vertex, 7);

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
        Vec3{-1.0f, 1.0f, 1.0f},    Vec3{-1.0f, 1.0f, -1.0f}, 
        Vec3{-1.0f, -1.0f, -1.0f},  Vec3{-1.0f, -1.0f, 1.0f}, 
        Vec3{1.0f, -1.0f, -1.0f},   Vec3{1.0f, -1.0f, 1.0f}, 
        Vec3{1.0f, 1.0f, -1.0f},    Vec3{1.0f, 1.0f, 1.86603f}, 
        Vec3{1.0f, 1.86603f, 1.0f}, Vec3{1.86603f, 1.0f, 1.0f}
	}, {
        {3, 0, 1, 2},
        {5, 3, 2, 4},
        {7, 8, 5, 4, 6},
        {9, 7, 6, 1, 0},
        {8, 9, 0, 3, 5},
        {6, 4, 2, 1},
        {8, 7, 9},
	});

	expect_bevel_vertex(mesh, vertex, vertex->normal(), 0.5, after);
});

/*
EDGE CASE

Initial mesh:
1---3
|\  |
| \ |
|  \|
0---2

Bevel Vertex on Vertex: 1 

After mesh:
  5-3
 /| |
1-4 |
|  \|
0---2
*/
Test test_a2_lx5_bevel_vertex_edge_boundary("a2.lx5.bevel_vertex.edge.boundary", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.5f},
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f}
	}, {
        {2, 0, 1}, 
        {3, 2, 1}
	});

	Halfedge_Mesh::VertexRef vertex = mesh.vertices.begin();
	std::advance(vertex, 1);

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
        Vec3{-0.5f, 0.0f, -0.5f},   Vec3{-0.5f, 0.0f, 0.25f}, 
        Vec3{0.5f, 0.0f, -0.5f},    Vec3{0.5f, 0.0f, 0.5f}, 
        Vec3{-0.25f, 0.0f, 0.25f},  Vec3{-0.25f, 0.0f, 0.5f}
	}, {
        {1, 4, 2, 0},
        {4, 5, 3, 2},
        {4, 1, 5}
	});

	expect_bevel_vertex(mesh, vertex, Vec3(0.0f, 1.0f, 0.0f), 0.25f, after);
});
