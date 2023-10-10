#include "test.h"

#include "geometry/util.h"
#include "lib/log.h"
#include "util/timer.h"

#include "geometry/halfedge.h"
#include "pathtracer/tri_mesh.h"
#include "scene/skeleton.h"

#include <map>
#include <sstream>
#include <unordered_set>

bool Test::run_generators = false;

namespace {
std::map<std::string, std::function<void()>>& get_tests() {
	static std::map<std::string, std::function<void()>> map;
	return map;
}

std::string to_lower(const std::string& str) {
	std::string result;
	for (auto c : str) {
		result += static_cast<decltype(c)>(std::tolower(c));
	}
	return result;
}
} // namespace

Test::Test(const std::string& name, const std::function<void()>& test) {
	auto ret = get_tests().emplace(to_lower(name), test);
	if (!ret.second) {
		die("Two tests named '%s'.", name.c_str());
	}
}

bool Test::run_tests(const std::string& prefix_) {

	std::string prefix = to_lower(prefix_);
	auto& tests = get_tests();

	// count the number of tests to run:
	uint32_t to_run = 0;
	for (auto& test : tests) {
		if (test.first.find(prefix) != std::string::npos) {
			++to_run;
		}
	}

	// run the tests:
	uint32_t passed = 0;
	uint32_t failed = 0;
	uint32_t ignored = 0;

	log("\nRunning %d tests including '%s':\n\n", to_run, prefix.c_str());
	for (auto& test : tests) {
		if (test.first.find(prefix) != std::string::npos) {
			log("\033[0;37m[%d/%d] \033[0;1m%s\033[0m...", passed + failed + 1, to_run,
			    test.first.c_str());
			try {
				test.second();
				log(" \033[32mPASSED\033[0m\n");
				passed += 1;
			} catch (Test::error& e) {
				log(" \033[31;1mFAILED\n    %s\033[0m\n", e.what());
				failed += 1;
			} catch (Test::ignored& e) {
				log(" \033[33;1mIGNORED\n    %s\033[0m\n", e.what());
				ignored += 1;
			}
		}
	}
	log("\n");
	if (passed || !failed) {
		log("\033[32mPASSED %d tests.\033[0m\n", passed);
	}
	if (failed) {
		log("\033[31;1mFAILED %d tests.\033[0m\n", failed);
	}
	if (ignored) {
		log("\033[33;1mIGNORED %d tests.\033[0m\n", ignored);
	}
	log("\n");

	return failed == 0;
}

std::string Test::mesh_to_string(const Halfedge_Mesh& mesh) {

	std::stringstream ss;
	std::unordered_map<uint32_t, uint32_t> vertex_id_to_idx;

	uint32_t i = 0;
	for (auto v = mesh.vertices.begin(); v != mesh.vertices.end(); ++v) {
		vertex_id_to_idx[v->id] = i++;
	}

	ss << "Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces({";
	for (auto v = mesh.vertices.begin(); v != mesh.vertices.end(); ++v) {
		auto format = [](float val) -> std::string {
			std::string ret = std::to_string(val);
			if (ret.find('.') == std::string::npos) ret = ret + ".0f";
			else ret += "f";
			return ret;
		};
		ss << "Vec3{" << format(v->position.x) << "," << format(v->position.y) << "," << format(v->position.z) << "}";
		if (v != std::prev(mesh.vertices.end())) {
			ss << ", ";
		}
	}
	ss << "}, {";
	bool first = true;
	for (auto f = mesh.faces.begin(); f != mesh.faces.end(); ++f) {
		if (f->boundary) continue;
		if (!first) ss << ",";
		first = false;
		ss << "{";
		auto h = f->halfedge;
		do {
			ss << vertex_id_to_idx[h->vertex->id];
			h = h->next;
			if (h != f->halfedge) {
				ss << ", ";
			}
		} while (h != f->halfedge);
		ss << "}";
	}
	ss << "});";

	return ss.str();
}

template<typename T>
static std::pair<float, float> time_op(T& _small, T& _large, const std::function<void(T&)>& op) {

	constexpr uint32_t rounds = 5;

	float small_time = std::numeric_limits<float>::infinity(),
		  large_time = std::numeric_limits<float>::infinity();

	std::vector<T> smalls, larges;
	for (uint32_t round = 0; round < rounds; round++) {
		smalls.push_back(_small.copy());
		larges.push_back(_large.copy());
	}

	for (uint32_t round = 0; round < rounds; round++) {
		{
			Timer large_t;
			op(larges[round]);
			large_time = std::min(large_time, large_t.ms());
		}
		{
			Timer small_t;
			op(smalls[round]);
			small_time = std::min(small_time, small_t.ms());
		}
	}

	return std::make_pair(small_time, large_time);
}

void Test::check_constant_time(std::function<void(Halfedge_Mesh&)> op) {
	static Halfedge_Mesh _small = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 0));
	static Halfedge_Mesh _large = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 4));

	auto [small_time, large_time] = time_op(_small, _large, op);

	constexpr float factor = 10.0f;
	if (factor * small_time < large_time) {
		throw Test::error("Operation not constant time. Small mesh: " + std::to_string(small_time) +
		                  "ms vs. Large mesh: " + std::to_string(large_time) + "ms.");
	}
}

void Test::check_linear_time(std::function<void(Halfedge_Mesh&)> op) {
	static Halfedge_Mesh _small = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 1));
	static Halfedge_Mesh _large = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 4));

	auto [small_time, large_time] = time_op(_small, _large, op);

	size_t small_total_elements = _small.vertices.size() + _small.edges.size() + _small.faces.size();
	size_t large_total_elements = _large.vertices.size() + _large.edges.size() + _large.faces.size();
	float ratio =
		static_cast<float>(large_total_elements) / static_cast<float>(small_total_elements);

	constexpr float factor = 2.0f;
	if (factor * ratio * small_time < large_time) {
		throw Test::error("Operation not linear time. Small mesh: " + std::to_string(small_time) +
		                  "ms vs. Large mesh: " + std::to_string(large_time) + "ms.");
	}
}

void Test::check_loglinear_time(std::function<void(Indexed_Mesh&)> op) {
	static Indexed_Mesh _small = Util::closed_sphere_mesh(1.0f, 1);
	static Indexed_Mesh _large = Util::closed_sphere_mesh(1.0f, 4);

	auto [small_time, large_time] = time_op(_small, _large, op);

	float small = static_cast<float>(_small.vertices().size());
	float large = static_cast<float>(_large.vertices().size());

	float ratio = (large * std::log2(large)) / (small * std::log2(small));

	constexpr float factor = 2.0f;
	if (factor * ratio * small_time < large_time) {
		throw Test::error(
			"Operation not log-linear time. Small mesh: " + std::to_string(small_time) +
			"ms vs. Large mesh: " + std::to_string(large_time) + "ms.");
	}
}

void Test::check_log_time(std::function<void(PT::Tri_Mesh&)> op) {
	static PT::Tri_Mesh _small = PT::Tri_Mesh(Util::closed_sphere_mesh(1.0f, 1), true);
	static PT::Tri_Mesh _large = PT::Tri_Mesh(Util::closed_sphere_mesh(1.0f, 4), true);

	auto [small_time, large_time] = time_op(_small, _large, op);

	float small = static_cast<float>(_small.n_triangles());
	float large = static_cast<float>(_large.n_triangles());

	float ratio = std::log2(large) / std::log2(small);

	constexpr float factor = 5.0f;
	if (factor * ratio * small_time < large_time) {
		throw Test::error(
			"Operation not log time. Small mesh: " + std::to_string(small_time) +
			"ms vs. Large mesh: " + std::to_string(large_time) + "ms.");
	}
}

double Test::total_squared_error(const std::vector<double>& a, const std::vector<double>& b) {
	assert(a.size() == b.size());
	double error = 0.0;
	for (uint32_t i = 0; i < a.size(); ++i) {
		if (a[i] == 0.0) continue;
		double e = a[i] - b[i];
		error += e * e / a[i];
	}
	return error;
}

double Test::total_squared_error(Spectrum a, Spectrum b) {
	double error = 0.0;
	for (uint32_t i = 0; i < 3; ++i) {
		if (a[i] == 0.0) continue;
		double e = a[i] - b[i];
		error += e * e / a[i];
	}
	return error;
}

double Test::print_empirical_threshold(const std::vector<double>& ref,
                                       const std::function<std::vector<double>()>& histogram) {

	constexpr uint32_t runs = 10000;

	std::vector<double> errors;
	for (uint32_t n = 0; n < runs; n++) {
		auto hist = histogram();
		errors.push_back(Test::total_squared_error(hist, ref));
	}

	std::sort(errors.begin(), errors.end(), std::greater<double>());

	log("\n\t0.1%% of runs had total squared error greater than: " +
	    std::to_string(errors[runs / 1000]) + "\n\t");

	return errors[runs / 1000];
}

bool Test::differs(float a, float b) {
	if (std::isnan(a) && std::isnan(b)) return false;
	if (std::isnan(a) || std::isnan(b)) return true;
	return std::abs(a-b) > differs_eps;
}

bool Test::differs(Vec2 a, Vec2 b) {
	return differs(a.x, b.x) || differs(a.y, b.y);
}

bool Test::differs(Vec3 a, Vec3 b) {
	return differs(a.x, b.x) || differs(a.y, b.y) || differs(a.z, b.z);
}

bool Test::differs(Vec4 a, Vec4 b) {
	return differs(a.x, b.x) || differs(a.y, b.y) || differs(a.z, b.z) || differs(a.w, b.w);
}

bool Test::differs(Spectrum a, Spectrum b) {
	return differs(a.to_vec(), b.to_vec());
}

bool Test::differs(std::vector<Vec3> a, std::vector<Vec3> b) {
	if (a.size() != b.size()) {
		return true;
	}
	for (size_t i = 0; i < a.size(); i++) {
		if (differs(a[i], b[i])) {
			return true;
		}
	}
	return false;
}

bool Test::differs(Mat4 a, Mat4 b) {
	for (int i = 0; i < 4; i++) {
		if (differs(a[i], b[i])) {
			return true;
		}
	}
	return false;
}

bool Test::differs(std::vector<Mat4> a, std::vector<Mat4> b) {
	if (a.size() != b.size()) {
		return true;
	}
	for (size_t i = 0; i < a.size(); i++) {
		if (differs(a[i], b[i])) {
			return true;
		}
	}
	return false;
}

bool Test::differs(std::vector<int> a, std::vector<int> b) {
	if (a.size() != b.size()) {
		return true;
	}
	for (size_t i = 0; i < a.size(); i++) {
		if (a[i] != b[i]) {
			return true;
		}
	}
	return false;
}

template< >
std::optional<std::string> Test::differs< Ray >(Ray const &a, Ray const &b) {
	if (differs(a.point, b.point)) {
		return "Origins do not match!";
	}
	if (differs(a.dir, b.dir)) {
		return "Directions do not match!";
	}
	if (a.depth != b.depth) {
		return "Depths do not match!";
	}

	return std::nullopt;
}

template< >
std::optional<std::string> Test::differs< PT::Trace >(PT::Trace const &a, PT::Trace const &b) {
	if (a.hit != b.hit) {
		return "Hit booleans do not match!";
	}

	// check other values if there was a hit
	if (a.hit) {
		if (differs(a.origin, b.origin)) {
			return "Hit origins do not match!";
		}
		if (std::abs(a.distance - b.distance) > EPS_F) {
			return "Hit distances do not match!";
		}
		if (differs(a.position, b.position)) {
			return "Hit positions do not match!";
		}
		if (differs(a.normal, b.normal)) {
			return "Hit normals do not match!";
		}
		if (differs(a.uv, b.uv)) {
			return "Hit UVs do not match!";
		}
	}

	return std::nullopt;
}

template< >
std::optional<std::string> Test::differs< Skeleton::Bone >(Skeleton::Bone const &a, Skeleton::Bone const &b) {

	if (differs(a.extent, b.extent)) {
		return "Bone extents do not match!";
	}
	if (differs(a.roll, b.roll)) {
		return "Bone rolls do not match!";
	}
	if (differs(a.radius, b.radius)) {
		return "Bone radii do not match!";
	}
	if (differs(a.pose, b.pose)) {
		return "Bone poses do not match!";
	}
	return std::nullopt;
}

struct Quantized_Vertices {

	Quantized_Vertices(float epsilon) : scale(2.0f / epsilon) {
	}

	std::tuple<int32_t, int32_t, int32_t> quantize(Vec3 v) {
		int32_t x = static_cast<int32_t>(std::floor(scale * v.x));
		int32_t y = static_cast<int32_t>(std::floor(scale * v.y));
		int32_t z = static_cast<int32_t>(std::floor(scale * v.z));
		return {x, y, z};
	}

	void insert(Vec3 v, uint32_t id) {

		// Very simple quantization; could probably improve precision by separating out the
		// fractional part. However, accuracy is not particularly important here: we just want to
		// be able to match vertices up to a small epsilon.

		auto q = quantize(v);
		if (vertices.find(q) != vertices.end()) {
			inputs_unique = false;
		}
		vertices.insert({q, id});
	}

	std::optional<uint32_t> find(Vec3 v) {

		auto [x, y, z] = quantize(v);
		for (int32_t dx = -1; dx <= 1; dx++) {
			for (int32_t dy = -1; dy <= 1; dy++) {
				for (int32_t dz = -1; dz <= 1; dz++) {
					if (vertices.find({x + dx, y + dy, z + dz}) != vertices.end()) {
						return vertices.at({x + dx, y + dy, z + dz});
					}
				}
			}
		}
		return std::nullopt;
	}

	bool all_unique() {
		if (!inputs_unique) return false;
		for (auto& [q, _] : vertices) {
			auto [x, y, z] = q;
			for (int32_t dx = -1; dx <= 1; dx++) {
				for (int32_t dy = -1; dy <= 1; dy++) {
					for (int32_t dz = -1; dz <= 1; dz++) {
						if (dx == 0 && dy == 0 && dz == 0) continue;
						if (vertices.find({x + dx, y + dy, z + dz}) != vertices.end()) {
							return false;
						}
					}
				}
			}
		}
		return true;
	}

	float scale;
	bool inputs_unique = true;
	std::map<std::tuple<int32_t, int32_t, int32_t>, uint32_t> vertices;
};


std::optional<std::string> Test::differs(Halfedge_Mesh const &source, Halfedge_Mesh const &target, CheckExtra check_extra) {
	//NOTE: check_extra actually ignored for now. Will update!

	constexpr float epsilon = 0.001f;

	using HalfedgeCRef = Halfedge_Mesh::HalfedgeCRef;
	using VertexCRef = Halfedge_Mesh::VertexCRef;
	using EdgeCRef = Halfedge_Mesh::EdgeCRef;
	using FaceCRef = Halfedge_Mesh::FaceCRef;

	Quantized_Vertices source_verts(epsilon);
	for (VertexCRef v = source.vertices.begin(); v != source.vertices.end(); v++) {
		source_verts.insert(v->position, v->id);
	}

	Quantized_Vertices target_verts(epsilon);
	for (VertexCRef v = target.vertices.begin(); v != target.vertices.end(); v++) {
		target_verts.insert(v->position, v->id);
	}

	if (!source_verts.all_unique())
		return "Source mesh does not have epsilon-unique vertex positions.";
	if (!target_verts.all_unique())
		return "Target mesh does not have epsilon-unique vertex positions.";

	std::unordered_map<uint32_t, VertexCRef> v_source_to_target;
	std::unordered_map<uint32_t, VertexCRef> v_target_to_source;

	for (VertexCRef v = source.vertices.begin(); v != source.vertices.end(); v++) {
		if (auto target_id = target_verts.find(v->position)) {
			v_target_to_source.insert({*target_id, v});
		} else {
			return "Source vertex set is not a subset of target vertex set!";
		}
	}

	for (VertexCRef v = target.vertices.begin(); v != target.vertices.end(); v++) {
		if (auto source_id = source_verts.find(v->position)) {
			v_source_to_target.insert({*source_id, v});
		} else {
			return "Target vertex set is not a subset of source vertex set!";
		}
	}

	// Edges are a bit more complicated because they're not necessarily unique: there can be
	// multiple edges across the same vertices. Hence we need to make sure to only
	// use each superset edge once when matching the subset.

	std::unordered_set<uint32_t> e_target_ids;
	std::unordered_set<uint32_t> e_source_ids;

	for (EdgeCRef e = source.edges.begin(); e != source.edges.end(); e++) {

		uint32_t id0 = e->halfedge->vertex->id;
		uint32_t id1 = e->halfedge->twin->vertex->id;
		VertexCRef v0 = v_source_to_target.at(id0);
		VertexCRef v1 = v_source_to_target.at(id1);

		bool found = false;
		HalfedgeCRef h = v0->halfedge;
		do {
			if (h->twin->vertex == v1) {
				auto match = h->edge;
				if (e_target_ids.find(match->id) == e_target_ids.end()) {
					found = true;
					e_target_ids.insert(match->id);
					break;
				}
			}
			h = h->twin->next;
		} while (h != v0->halfedge);

		if (!found) return "Source edge set is not a subset of target edge set!";
	}

	for (EdgeCRef e = target.edges.begin(); e != target.edges.end(); e++) {

		uint32_t id0 = e->halfedge->vertex->id;
		uint32_t id1 = e->halfedge->twin->vertex->id;
		VertexCRef v0 = v_target_to_source.at(id0);
		VertexCRef v1 = v_target_to_source.at(id1);

		bool found = false;
		HalfedgeCRef h = v0->halfedge;
		do {
			if (h->twin->vertex == v1) {
				auto match = h->edge;
				if (e_source_ids.find(match->id) == e_source_ids.end()) {
					found = true;
					e_source_ids.insert(match->id);
					break;
				}
			}
			h = h->twin->next;
		} while (h != v0->halfedge);

		if (!found) return "Target edge set is not a subset of source edge set!";
	}

	assert(e_source_ids.size() == source.edges.size());
	assert(e_target_ids.size() == target.edges.size());
	assert(e_source_ids.size() == e_target_ids.size());

	// There can also be multiple faces spanning the same vertex set, so we also need
	// to not re-use face IDs.
	// Also remember to check face boundary flags.

	std::unordered_set<uint32_t> f_target_ids;
	std::unordered_set<uint32_t> f_source_ids;

	for (FaceCRef f = source.faces.begin(); f != source.faces.end(); f++) {

		std::unordered_multiset<VertexCRef> verts;
		{
			HalfedgeCRef h = f->halfedge;
			do {
				verts.insert(v_source_to_target.at(h->vertex->id));
				h = h->next;
			} while (h != f->halfedge);
		}

		bool found = false;
		HalfedgeCRef begin = (*verts.begin())->halfedge;
		HalfedgeCRef h = begin;
		do {
			FaceCRef match = h->face;

			std::unordered_multiset<VertexCRef> remaining_verts = verts;
			HalfedgeCRef h_match = match->halfedge;
			do {
				auto v = remaining_verts.find(h_match->vertex);
				if (v != remaining_verts.end()) remaining_verts.erase(v);
				h_match = h_match->next;
			} while (h_match != match->halfedge);

			if (remaining_verts.empty() && f_target_ids.find(match->id) == f_target_ids.end() &&
			    match->boundary == f->boundary) {
				found = true;
				f_target_ids.insert(match->id);
				break;
			}

			h = h->twin->next;
		} while (h != begin);

		if (!found) return "Source face set is not a subset of target face set!";
	}

	for (FaceCRef f = target.faces.begin(); f != target.faces.end(); f++) {

		std::unordered_multiset<VertexCRef> verts;
		{
			HalfedgeCRef h = f->halfedge;
			do {
				verts.insert(v_target_to_source.at(h->vertex->id));
				h = h->next;
			} while (h != f->halfedge);
		}

		bool found = false;
		HalfedgeCRef begin = (*verts.begin())->halfedge;
		HalfedgeCRef h = begin;
		do {
			FaceCRef match = h->face;

			std::unordered_multiset<VertexCRef> remaining_verts = verts;
			HalfedgeCRef h_match = match->halfedge;
			do {
				auto v = remaining_verts.find(h_match->vertex);
				if (v != remaining_verts.end()) remaining_verts.erase(v);
				h_match = h_match->next;
			} while (h_match != match->halfedge);

			if (remaining_verts.empty() && f_source_ids.find(match->id) == f_source_ids.end() &&
			    match->boundary == f->boundary) {
				found = true;
				f_source_ids.insert(match->id);
				break;
			}

			h = h->twin->next;
		} while (h != begin);

		if (!found) return "Target face set is not a subset of source face set!";
	}

	assert(f_source_ids.size() == source.faces.size());
	assert(f_target_ids.size() == target.faces.size());
	assert(f_source_ids.size() == f_target_ids.size());

	return std::nullopt;
}

void Test::print_matrix(Mat4 matrix) {
	std::stringstream ss;
	ss << "Mat4{";
	for (int i = 0; i < 4; i++) {
		ss << "Vec4{";
		for (int j = 0; j < 4; j++) {
			ss << std::to_string(matrix[i][j]) << "f";
			if (j != 3) {
				ss << ", ";
			}
		}
		ss << "}";
		if (i != 3) {
			ss << ", ";
		}
	}
	ss << "}";
	info("%s", ss.str().c_str());
}

void Test::print_vec3s(const std::vector<Vec3>& vec) {
	std::stringstream ss;
	ss << "std::vector{";
	for (size_t i = 0; i < vec.size(); i++) {
		ss << "Vec3{";
		ss << std::to_string(vec[i].x) << "f, ";
		ss << std::to_string(vec[i].y) << "f, ";
		ss << std::to_string(vec[i].z) << "f}";
		if (i != vec.size() - 1) {
			ss << ", ";
		}
	}
	ss << "}";
	info("%s", ss.str().c_str());
}

void Test::print_floats(const std::vector<float>& vec) {
	std::stringstream ss;
	ss << "std::vector{";
	for (size_t i = 0; i < vec.size(); i++) {
		ss << std::to_string(vec[i]) << "f";
		if (i != vec.size() - 1) {
			ss << ", ";
		}
	}
	ss << "}";
	info("%s", ss.str().c_str());
}

void Test::print_spectrums(const std::vector<Spectrum>& vec) {
	std::stringstream ss;
	ss << "std::vector{";
	for (size_t i = 0; i < vec.size(); i++) {
		ss << "Spectrum{";
		ss << std::to_string(vec[i].r) << "f, ";
		ss << std::to_string(vec[i].g) << "f, ";
		ss << std::to_string(vec[i].b) << "f}";
		if (i != vec.size() - 1) {
			ss << ", ";
		}
	}
	ss << "}";
	info("%s", ss.str().c_str());
}

bool Test::distant_from(const Halfedge_Mesh& from, const Halfedge_Mesh& to, float scale) {

	for (auto v = to.vertices.begin(); v != to.vertices.end(); ++v) {

		// This is highly approximate: we simply scale up the threshold by the mean edge length
		// incident from this vertex. This should make the distance test roughly scale-independent,
		// but we still need to manually adjust the scale parameter for specific tests.

		uint32_t n = 0;
		float avg_edge = 0.0f;
		auto h = v->halfedge;
		do {
			n++;
			avg_edge += (h->twin->vertex->position - v->position).norm();
			h = h->twin->next;
		} while (h != v->halfedge);
		avg_edge /= static_cast<float>(n);

		if (closest_distance(from, v->position) > avg_edge * scale * 0.1f) {
			return true;
		}
	}

	return false;
}

float Test::closest_distance(const Halfedge_Mesh& from, const Vec3& to) {
	float d = std::numeric_limits<float>::infinity();

	// https://iquilezles.org/articles/triangledistance/
	auto length2 = [](Vec3 v) { return dot(v, v); };
	auto distance_to_triangle = [length2](Vec3 p, Vec3 v1, Vec3 v2, Vec3 v3) {
		Vec3 v21 = v2 - v1;
		Vec3 p1 = p - v1;
		Vec3 v32 = v3 - v2;
		Vec3 p2 = p - v2;
		Vec3 v13 = v1 - v3;
		Vec3 p3 = p - v3;
		Vec3 nor = cross(v21, v13);
		return std::sqrt(
			(sign(dot(cross(v21, nor), p1)) + sign(dot(cross(v32, nor), p2)) +
		         sign(dot(cross(v13, nor), p3)) <
		     2.0f)
				? std::min(std::min(length2(v21 * std::clamp(dot(v21, p1) / length2(v21), 0.0f, 1.0f) - p1),
		                            length2(v32 * std::clamp(dot(v32, p2) / length2(v32), 0.0f, 1.0f) - p2)),
		                   length2(v13 * std::clamp(dot(v13, p3) / length2(v13), 0.0f, 1.0f) - p3))
				: dot(nor, p1) * dot(nor, p1) / length2(nor));
	};

	for (Halfedge_Mesh::FaceCRef face = from.faces.begin(); face != from.faces.end(); ++face) {
		if (face->boundary) continue;
		Halfedge_Mesh::HalfedgeCRef h = face->halfedge;
		Vec3 v1 = h->vertex->position;
		h = h->next;
		do {
			Vec3 v2 = h->vertex->position;
			h = h->next;
			Vec3 v3 = h->vertex->position;
			d = std::min(d, distance_to_triangle(to, v1, v2, v3));
		} while (h != face->halfedge);
	}

	return d;
}
