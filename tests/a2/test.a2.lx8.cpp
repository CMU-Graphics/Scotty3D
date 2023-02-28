#include "test.h"
#include "geometry/halfedge.h"

static void expect_weld_edge(Halfedge_Mesh &mesh, Halfedge_Mesh::EdgeRef edge, Halfedge_Mesh::EdgeRef edge2, Halfedge_Mesh const &after) {
	if (auto ret = mesh.weld_edges(edge, edge2)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}
		// check if returned edge is the same edge
		if (ret != edge) {
			throw Test::error("Did not return the first edge!");
		}
		// check if returned edge is not a boundary edge
		if (ret.value()->on_boundary()) {
			throw Test::error("Did not make the first edge a non-boundary edge!");
		}
		// check mesh shape:
		if (auto difference = Test::differs(mesh, after, Test::CheckAllBits)) {
			puts("");
			log("%s", mesh.describe().c_str());
			log("Wanted: %s", after.describe().c_str());
			throw Test::error("Resulting mesh did not match expected: " + *difference);
		}
	} else {
		throw Test::error("weld_edges rejected operation!");
	}
}

/*
BASIC CASE

Initial mesh:
0---1 2\
|   | | \
|   | |  3
|   | | /
4---5 6/

Weld Edges on Edges: 1-5 and 2-4

After mesh:
0---1\     
|   | \
|   |  2
|   | /
3---4/
*/
Test test_a2_lx8_weld_edges_basic_simple("a2.lx8.weld_edges.basic.simple", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.0f, 1.0f, 0.0f), Vec3(1.1f, 1.0f, 0.0f),
		                                                         Vec3(2.2f, 0.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.1f,-1.0f, 0.0f), Vec3(1.4f, -1.0f, 0.0f)
	}, {
		{0, 4, 5, 1}, 
		{2, 6, 3}
	});
	Halfedge_Mesh::EdgeRef edge = mesh.faces.begin()->halfedge->next->next->edge;
	Halfedge_Mesh::EdgeRef edge2 = std::next(mesh.faces.begin())->halfedge->edge;

	Halfedge_Mesh after = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.05f, 1.0f, 0.0f),
		                                            Vec3(2.2f, 0.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.25f, -1.0f, 0.0f)
	}, {
		{0, 3, 4, 1}, 
		{1, 4, 2}
	});

	expect_weld_edge(mesh, edge, edge2, after);
});

/*
EDGE CASE

Initial mesh:
0---1\     
|   | \
|   |  2
|   | /
3---4/

Extrude Face on Face: 1-5 and 1-3
*/
Test test_a2_lx8_weld_edges_edge_boundary("a2.lx8.weld_edges.edge.boundary", []() {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({
		Vec3(-1.0f, 1.1f, 0.0f), Vec3(1.05f, 1.0f, 0.0f),
		                                            Vec3(2.2f, 0.0f, 0.0f),
		Vec3(-1.3f,-0.7f, 0.0f), Vec3(1.25f, -1.0f, 0.0f)
	}, {
		{0, 3, 4, 1}, 
		{1, 4, 2}
	});

	Halfedge_Mesh::EdgeRef edge = mesh.halfedges.begin()->next->next->edge;
	Halfedge_Mesh::EdgeRef edge2 = std::next(mesh.faces.begin())->halfedge->next->edge;

	if (mesh.weld_edges(edge, edge2)) {
		throw Test::error("EDGE CASE: Did not reject welding a non-boundary edge!");
	}
});
