
#include "test.h"
#include "geometry/halfedge.h"
#include "geometry/util.h"

static void expect_simplify(Halfedge_Mesh& mesh, float ratio, bool check_ratio = true) {

	float expected_faces = mesh.faces.size() * ratio;

	if (mesh.simplify(ratio)) {
		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh: " + msg.value().second);
		}

		if (check_ratio && std::abs(expected_faces - mesh.faces.size()) > 1.0f) {
			throw Test::error("Simplification did not remove the expected number of faces!");
		}

	} else {
		throw Test::error("Simplification rejected!");
	}
}

// simplify a ball a little
Test test_meshedit_a2_go3_simplify_ball_10("meshedit.a2.go3.simplify.ball.10", []() {
	Halfedge_Mesh ball = Halfedge_Mesh::from_indexed_mesh(Util::sphere_mesh(1.0f, 2));
	Halfedge_Mesh orig = ball.copy();

	expect_simplify(ball, 0.9f);

	if (Test::distant_from(ball, orig, 0.75f)) {
		throw Test::error("Simplify didn't preserve mesh shape!");
	}
});

// simplify a ball a bit
Test test_meshedit_a2_go3_simplify_ball_50("meshedit.a2.go3.simplify.ball.50", []() {
	Halfedge_Mesh ball = Halfedge_Mesh::from_indexed_mesh(Util::sphere_mesh(1.0f, 2));
	Halfedge_Mesh orig = ball.copy();

	expect_simplify(ball, 0.5f);

	if (Test::distant_from(ball, orig, 1.0f)) {
		throw Test::error("Simplify didn't preserve mesh shape!");
	}
});

// simplify a ball a lot
Test test_meshedit_a2_go3_simplify_ball_90("meshedit.a2.go3.simplify.ball.90", []() {
	Halfedge_Mesh ball = Halfedge_Mesh::from_indexed_mesh(Util::sphere_mesh(1.0f, 2));
	Halfedge_Mesh orig = ball.copy();

	expect_simplify(ball, 0.1f);

	if (Test::distant_from(ball, orig, 7.5f)) {
		throw Test::error("Simplify didn't preserve mesh shape!");
	}
});
