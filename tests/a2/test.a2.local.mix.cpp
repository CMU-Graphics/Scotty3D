
#include "test.h"
#include "geometry/halfedge.h"
#include "geometry/util.h"
#include "util/rand.h"

Test test_a2_local_mix("a2.local.mix", []() {
	RNG rng(2266524198);

	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 2));

	constexpr uint32_t operations = 10;

	// On a list iterator, std::advance is O(n), making this O(n^2).
	// It would be nice if we had a halfedge mesh implementation with flat arrays instead of std::lists...
	auto vertex = [&]() {
		int32_t idx = rng.integer(0, static_cast<int32_t>(mesh.vertices.size()));
		auto itr = mesh.vertices.begin();
		std::advance(itr, idx);
		return itr;
	};
	auto edge = [&]() {
		int32_t idx = rng.integer(0, static_cast<int32_t>(mesh.edges.size()));
		auto itr = mesh.edges.begin();
		std::advance(itr, idx);
		return itr;
	};
	auto face = [&]() {
		int32_t idx = rng.integer(0, static_cast<int32_t>(mesh.faces.size()));
		auto itr = mesh.faces.begin();
		std::advance(itr, idx);
		return itr;
	};

    auto positions = [&](Halfedge_Mesh::FaceRef f) {
		Halfedge_Mesh::HalfedgeRef face_he = f->halfedge;
		Halfedge_Mesh::HalfedgeRef face_he_orig = face_he;
        std::vector<Vec3> start_positions;
		do {
            start_positions.push_back(face_he->vertex->position);
			face_he = face_he->next;
		} while (face_he != face_he_orig);
        return start_positions;
    };

	for (uint32_t i = 0; i < operations; i++) {
		int32_t op = rng.integer(0, 11);
		switch (op) {
		case 0: {
			auto e = edge();
			mesh.split_edge(e);
		} break;
		case 1: {
			auto e = edge();
			mesh.flip_edge(e);
		} break;
		case 2: {
			auto e = edge();
			mesh.collapse_edge(e);
		} break;
		case 3: {
			auto f = face();
			if(auto ret = mesh.extrude_face(f)) {
                mesh.extrude_positions(ret.value(), ret.value()->normal(), 0.5f);
            }
		} break;
		case 4: {
			auto e = edge();
			mesh.dissolve_edge(e);
		} break;
		case 5: {
			auto f = face();
			mesh.collapse_face(f);
		} break;
		case 6: {
			auto f = face();
			mesh.inset_vertex(f);
		} break;
		case 7: {
			auto e = edge();
			mesh.bisect_edge(e);
		} break;
		case 8: {
			auto v = vertex();
			if(auto ret = mesh.bevel_vertex(v)) {
                mesh.bevel_positions(ret.value(), positions(ret.value()), ret.value()->normal(), 0.5f);
            }
		} break;
		case 9: {
			auto v = vertex();
			mesh.dissolve_vertex(v);
		} break;
		case 10: {
			auto e = edge();
			if(auto ret = mesh.bevel_edge(e)) {
                mesh.bevel_positions(ret.value(), positions(ret.value()), ret.value()->normal(), 0.5f);
            }
		} break;
		default: die("Got bad random operation?");
		}

		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh after operation " + std::to_string(op) + " with seed " +
			                  std::to_string(rng.get_seed()) + ": " + msg.value().second);
		}
	}
});
