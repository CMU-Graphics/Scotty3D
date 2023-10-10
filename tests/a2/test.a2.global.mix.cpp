
#include "test.h"
#include "geometry/halfedge.h"
#include "geometry/util.h"
#include "util/rand.h"

Test test_a2_global_mix("a2.global.mix", []() {
	RNG rng(2266524198);

	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 2));

	constexpr uint32_t operations = 10;

	for (uint32_t i = 0; i < operations; i++) {

		int32_t op = rng.integer(0, 4);

		switch (op) {
		case 0: {
			mesh.triangulate();
		} break;
		case 1: {
			mesh.simplify(0.25f);
		} break;
		case 2: {
			mesh.isotropic_remesh({3, 1.2f, 0.8f, 10, 0.2f});
		} break;
		case 3: {
			int32_t type = rng.integer(0, 2);
			if (type == 0)
				mesh.loop_subdivide();
			else if (type == 1)
				mesh.linear_subdivide();
			else
				mesh.catmark_subdivide();
		} break;
		default: die("Got bad random operation?");
		}

		if (auto msg = mesh.validate()) {
			throw Test::error("Invalid mesh after operation " + std::to_string(op) + " with seed " +
			                  std::to_string(rng.get_seed()) + ": " + msg.value().second);
		}
	}
});
