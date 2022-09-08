
#include "particles.h"

bool Particles::update(const World_Info& world, Particles::Particle& p) {

    // TODO(Animation): Task 4

    // Compute the trajectory of this particle for the next dt seconds.

    // (1) Build a ray representing the particle's path if it travelled at constant velocity.

    // (2) Intersect the ray with the scene and account for collisions. Be careful when placing
    // collision points using the particle radius. Move the particle to its next position.

    // (3) Account for acceleration due to gravity.

    // (4) Repeat until the entire time step has been consumed.

    // (5) Decrease the particle's age and return whether it should die.

    return false;
}

void Particles::advance(const PT::Aggregate& scene, const Mat4& to_world, float r, float dt) {
	
    if(step_size < EPS_F) return;

    last_update += dt;

    while(last_update > step_size) {
        step(scene, to_world, r);
        last_update -= step_size;
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

    particle_cooldown -= step_size;
    float cos = std::cos(Radians(spread_angle) / 2.0f);

    if(pps > 0.0f) {
        
        double cooldown = 1.0 / pps;
        while(particle_cooldown <= 0.0f) {

            float z = lerp(cos, 1.0f, RNG::unit());
            float t = 2 * PI_F * RNG::unit();
            float d = std::sqrt(1 - z * z);
            Vec3 dir = initial_velocity * Vec3(d * std::cos(t), z, d * std::sin(t));

            Particle p;
            p.velocity = dir;
            p.age = lifetime;
            next.push_back(p);

            particle_cooldown += cooldown;
        }
    }
    particles = std::move(next);
}

void Particles::clear() {
	particles.clear();
}

bool operator!=(const Particles& a, const Particles& b) {
	return a.gravity != b.gravity || a.scale != b.scale ||
	       a.initial_velocity != b.initial_velocity || a.spread_angle != b.spread_angle ||
	       a.lifetime != b.lifetime || a.pps != b.pps || a.step_size != b.step_size;
}
