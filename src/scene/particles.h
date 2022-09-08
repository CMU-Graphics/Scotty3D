
#pragma once

#include <memory>
#include <variant>

#include "../lib/mathlib.h"
#include "../pathtracer/aggregate.h"

class Particles {
public:
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

	void clear();
	void advance(const PT::Aggregate& scene, const Mat4& to_world, float r, float dt);

	float gravity = 9.8f;
	float scale = 0.1f;
	float initial_velocity = 5.0f;
	float spread_angle = 0.0f;
	float lifetime = 2.5f;
	float pps = 10.0f;
	float step_size = 0.01f;
	
	std::vector<Particle> particles;

private:
	void step(const PT::Aggregate& scene, const Mat4& to_world, float r);
	double last_update = 0.0f;
	double particle_cooldown = 0.0f;
};

bool operator!=(const Particles& a, const Particles& b);
