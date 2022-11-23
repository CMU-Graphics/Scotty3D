
#include "particles.h"

bool Particles::update(const World_Info& world, Particles::Particle& p) {

    //A4T4: particle update

    // Compute the trajectory of this particle for the next dt seconds.

    // (1) Build a ray representing the particle's path as if it travelled at constant velocity.

    // (2) Intersect the ray with the scene and account for collisions. Be careful when placing
    // collision points using the particle radius. Move the particle to its next position.

    // (3) Account for acceleration due to gravity.

    // (4) Repeat until the entire time step has been consumed.

    // (5) Decrease the particle's age and return whether it should die.

    return false;
}

void Particles::advance(const PT::Aggregate& scene, const Mat4& to_world, float r, float dt) {
	
    if(step_size < EPS_F) return;

    step_accum += dt;

    while(step_accum > step_size) {
        step(scene, to_world, r);
        step_accum -= step_size;
    }
}

void Particles::step(const PT::Aggregate& scene, const Mat4& to_world, float r) {

    std::vector<Particle> next;
    next.reserve(particles.size());

    Mat4 to_local = to_world.inverse();
    World_Info info {scene, to_world, to_local, to_world.T(), step_size, r};

    for(Particle& p : particles) {
        if(update(info, p)) {
            next.emplace_back(p);
        }
    }

    if(rate > 0.0f) {

		//helpful when emitting particles:
    	float cos = std::cos(Radians(spread_angle) / 2.0f);

		//will emit particle i when i == time * rate
		//(i.e., will emit particle when time * rate hits an integer value.)
		//so need to figure out all integers in [current_step, current_step+1) * step_size * rate
		//compute the range:
		double begin_t = current_step * double(step_size) * double(rate);
		double end_t = (current_step + 1) * double(step_size) * double(rate);

		uint64_t begin_i = uint64_t(std::max(0.0, std::ceil(begin_t)));
		uint64_t end_i = uint64_t(std::max(0.0, std::ceil(end_t)));

		//iterate all integers in [begin, end):
		for (uint64_t i = begin_i; i < end_i; ++i) {
			//spawn particle 'i':

            float z = lerp(cos, 1.0f, rng.unit());
            float t = 2 * PI_F * rng.unit();
            float d = std::sqrt(1.0f - z * z);
            Vec3 dir = Vec3(d * std::cos(t), d * std::sin(t), initial_velocity * z);

            Particle p;
            p.velocity = dir;
            p.age = lifetime; //NOTE: could adjust lifetime based on index
            next.push_back(p);
        }
    }

    particles = std::move(next);
	current_step += 1;
}

void Particles::reset() {
	particles.clear();
	step_accum = 0.0f;
	current_step = 0;
	rng.seed(seed);
}

bool operator!=(const Particles& a, const Particles& b) {
	return a.gravity != b.gravity || a.scale != b.scale ||
	       a.initial_velocity != b.initial_velocity || a.spread_angle != b.spread_angle ||
	       a.lifetime != b.lifetime || a.rate != b.rate || a.step_size != b.step_size || a.seed != b.seed;
}
