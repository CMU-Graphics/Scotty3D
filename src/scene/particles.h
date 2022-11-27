
#pragma once

#include <memory>
#include <variant>

#include "../lib/mathlib.h"
#include "../pathtracer/aggregate.h"

class Particles {
public:
	Particles() { reset(); }

	struct Particle {
		//note: position and velocity are always in world (not system-local) space:
		Vec3 position;
		Vec3 velocity;
		float age;

		//called by Particles::step(); returns 'true' if particle should be kept for next frame:
		bool update(const PT::Aggregate &scene, Vec3 const &gravity, const float radius, const float dt);

		//- - - - - - - - - - - - -
		template< Intent I, typename F, typename T >
		static void introspect(F&& f, T&& t) {
			if constexpr (I != Intent::Animate) f("position", t.position);
			if constexpr (I != Intent::Animate) f("velocity", t.velocity);
			if constexpr (I != Intent::Animate) f("age", t.age);
		}
		static inline const char *TYPE = "Particle";
	};

	void reset(); //reset to time = 0
	void advance(const PT::Aggregate& scene, const Mat4& to_world, float dt);

	Vec3 gravity = Vec3(0.0f, -9.8f, 0.0f); //in world coordinates
	float radius = 0.1f; //radius of particles, in world units
	float initial_velocity = 5.0f; //along local y axis
	float spread_angle = 0.0f;
	float lifetime = 2.5f;
	float rate = 10.0f; //particles emitted per second

	float step_size = 0.01f; //simulation step size
	uint32_t seed = 0x31415926; //RNG seed
	
	std::vector<Particle> particles;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("gravity", t.gravity);
		f("radius", t.radius);
		f("initial_velocity", t.initial_velocity);
		f("spread_angle", t.spread_angle);
		f("lifetime", t.lifetime);
		f("rate", t.rate);
		f("step_size", t.step_size);
		if constexpr (I != Intent::Animate) f("seed", t.seed);

		if constexpr (I != Intent::Animate) f("particles", t.particles); //these probably don't need to be read/written
	}
	static inline const char *TYPE = "Particles";

private:
	float step_accum = 0.0f; //accumulated time toward next step, used by advance()
	void step(const PT::Aggregate& scene, const Mat4& to_world);
	uint64_t current_step = 0; //steps run so far, used by step() to determine how many particles to spawn
	RNG rng;
};

bool operator!=(const Particles& a, const Particles& b);
