
#include "test.h"
#include "geometry/halfedge.h"
#include "geometry/util.h"

static float stddev(const std::vector<float>& samples) {
	float mean = 0.0f;
	for (auto sample : samples) {
		mean += sample;
	}
	mean /= samples.size();

	float variance = 0.0f;
	for (auto sample : samples) {
		variance += (sample - mean) * (sample - mean);
	}
	variance /= samples.size();

	return std::sqrt(variance);
}

static void expect_remesh(Halfedge_Mesh& mesh, float fcurve, float fdefect, float flength, float farea) {

	float old_curvature = 0.0f;
	float old_avg_defect = 0.0f;
	for (auto v = mesh.vertices.begin(); v != mesh.vertices.end(); ++v) {
		old_curvature += v->gaussian_curvature();
		old_avg_defect += std::abs(static_cast<int32_t>(v->degree()) - 6);
	}
	old_avg_defect /= mesh.vertices.size();

	std::vector<float> old_edge_lengths;
	for (auto e = mesh.edges.begin(); e != mesh.edges.end(); ++e) {
		old_edge_lengths.push_back(e->length());
	}
	float old_edge_stddev = stddev(old_edge_lengths);

	std::vector<float> old_face_areas;
	for (auto f = mesh.faces.begin(); f != mesh.faces.end(); ++f) {
		old_face_areas.push_back(f->area());
	}
	float old_face_stddev = stddev(old_face_areas);

	mesh.isotropic_remesh({1, 1.2f, 0.8f, 5, 0.2f});

	if (auto msg = mesh.validate()) {
		throw Test::error("Invalid mesh: " + msg.value().second);
	}

	float new_curvature = 0.0f;
	float new_avg_defect = 0.0f;
	for (auto v = mesh.vertices.begin(); v != mesh.vertices.end(); ++v) {
		new_curvature += v->gaussian_curvature();
		new_avg_defect += std::abs(static_cast<int32_t>(v->degree()) - 6);
	}
	new_avg_defect /= mesh.vertices.size();

	std::vector<float> new_edge_lengths;
	for (auto e = mesh.edges.begin(); e != mesh.edges.end(); ++e) {
		new_edge_lengths.push_back(e->length());
	}
	float new_edge_stddev = stddev(new_edge_lengths);

	std::vector<float> new_face_areas;
	for (auto f = mesh.faces.begin(); f != mesh.faces.end(); ++f) {
		new_face_areas.push_back(f->area());
	}
	float new_face_stddev = stddev(new_face_areas);

	if (new_curvature > old_curvature * fcurve) {
		throw Test::error("Remesh did not decrease total curvature by a factor of " + std::to_string(fcurve));
	}

	if (new_avg_defect > old_avg_defect * fdefect) {
		throw Test::error("Remesh did not decrease average vertex degree defect by a factor of " + std::to_string(fdefect));
	}

	if (new_edge_stddev > old_edge_stddev * flength) {
		throw Test::error("Remesh did not decrease edge length deviation by a factor of " + std::to_string(flength));
	}

	if (new_face_stddev > old_face_stddev * farea) {
		throw Test::error("Remesh did not decrease face area deviation by a factor of " + std::to_string(farea));
	}
}

// isotropic remeshing on a good ball
Test test_a2_go2_remesh_basic_ball_good("a2.go2.remesh.basic.ball.good", []() {
	Halfedge_Mesh ball = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 1));
	Halfedge_Mesh remesh = ball.copy();

	expect_remesh(ball, 1.0f, 1.0f, 1.0f, 1.0f);

	// check mesh shape:
	if (Test::distant_from(ball, remesh, 0.1f)) {
		throw Test::error("Remesh didn't preserve mesh shape!");
	}
});

// isotropic remeshing on a zero(?) radius ball
Test test_a2_go2_remesh_basic_zero("a2.go2.remesh.basic.zero", []() {
	Halfedge_Mesh ball = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 0));
	Halfedge_Mesh remesh = ball.copy();

	ball.isotropic_remesh({0, 1.2f, 0.8f, 5, 0.2f});
	
	// check mesh shape:
	if (auto diff = Test::differs(ball, remesh)) {
		throw Test::error("Remesh with zero iterations changed the mesh: " + diff.value());
	}
});
