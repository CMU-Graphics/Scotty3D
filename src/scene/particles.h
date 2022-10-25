
#pragma once

#include <memory>
#include <variant>

#include "../lib/mathlib.h"
#include "../pathtracer/aggregate.h"

class Particles {
public:
	Particles() { reset(); }

	struct Particle {
		Vec3 position;
		Vec3 velocity;
		float age;
	};

	struct World_Info {
		const PT::Aggregate& scene;
		Mat4 to_world, to_local, to_local_normal;
		float dt = 0.0f;
		float radius = 1.0f;
	};
	bool update(const World_Info& world, Particle& p);

	void reset(); //reset to time = 0
	void advance(const PT::Aggregate& scene, const Mat4& to_world, float r, float dt);

	Vec3 gravity = Vec3(0.0f, -9.8f, 0.0f); //in world coordinates
	float scale = 0.1f;
	float initial_velocity = 5.0f; //along local z axis, canonically
	float spread_angle = 0.0f;
	float lifetime = 2.5f;
	float rate = 10.0f; //particles emitted per second

	float step_size = 0.01f; //simulation step size
	uint32_t seed = 0x31415926; //RNG seed
	
	std::vector<Particle> particles;

private:
	float step_accum = 0.0f; //accumulated time toward next step, used by advance()
	void step(const PT::Aggregate& scene, const Mat4& to_world, float r);
	uint64_t current_step = 0; //steps run so far, used by step() to determine how many particles to spawn
	RNG rng;
};

bool operator!=(const Particles& a, const Particles& b);
