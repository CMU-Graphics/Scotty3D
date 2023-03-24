#include "test.h"
#include "geometry/util.h"
#include "lib/mathlib.h"
#include "pathtracer/aggregate.h"
#include "scene/particles.h"
#include "util/rand.h"

Test test_a4_task4_particles_free_fall("a4.task4.particles.free_fall", []() {
	PT::Aggregate empty;
	Particles particles;
	particles.radius = 0.2f;
	particles.particles = {{Vec3(0.0f), Vec3(0.0f), 1.0f}};

	while (particles.particles[0].update(empty, particles.gravity, particles.radius, 0.01f)) {
	}

	Vec3 expected_pos = Vec3(0.000000f, -4.949000f, 0.000000f);
	Vec3 expected_velocity = Vec3(0.000000f, -9.897995f, 0.000000f);
	Vec3 actual_pos = particles.particles[0].position;
	Vec3 actual_velocity = particles.particles[0].velocity;
	if (Test::differs(expected_pos, actual_pos)) {
		throw Test::error("Particle position differs from expected value!");
	} else if (Test::differs(expected_velocity, actual_velocity)) {
		throw Test::error("Particle velocity differs from expected value!");
	}
});

Test test_a4_task4_particles_ground_only("a4.task4.particles.ground_only", []() {
	PT::Tri_Mesh ground_mesh{Util::square_mesh(10), false};
	PT::Aggregate ground{PT::List{std::vector{PT::Instance{&ground_mesh, nullptr, Mat4::I}}}};

	Particles particles;
	particles.radius = 0.2f;
	particles.particles = {{Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f), 1.0f}};

	while (particles.particles[0].update(ground, particles.gravity, particles.radius, 0.01f)) {
	}

	Vec3 expected_pos = Vec3(0.000000f, 0.762118f, 0.000000f);
	Vec3 expected_velocity = Vec3(0.000000f, -2.058001f, 0.000000f);
	Vec3 actual_pos = particles.particles[0].position;
	Vec3 actual_velocity = particles.particles[0].velocity;
	if (Test::differs(expected_pos, actual_pos)) {
		throw Test::error("Particle position differs from expected value!");
	} else if (Test::differs(expected_velocity, actual_velocity)) {
		throw Test::error("Particle velocity differs from expected value!");
	}
});
