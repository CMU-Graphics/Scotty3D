
#include <imgui/imgui_custom.h>

#include "../geometry/util.h"
#include "../platform/renderer.h"
#include "../scene/undo.h"

#include "manager.h"
#include "simulate.h"

namespace Gui {

Simulate::Simulate() : thread_pool(std::thread::hardware_concurrency()) {
	sim_timer.reset();
}

Simulate::~Simulate() {
	thread_pool.stop();
}

void Simulate::step(Scene& scene, float dt) {
    for(auto& [_, inst] : scene.instances.particles) {
		if(inst->settings.simulate_here && !inst->particles.expired()) {
			Mat4 to_world = inst->transform.expired() ? Mat4::I : inst->transform.lock()->local_to_world();
        	inst->particles.lock()->advance(collision.world, to_world, dt);
		}
    }
}

void Simulate::pause() {
	sim_timer.pause();
}

void Simulate::resume() {
	sim_timer.unpause();
}

void Simulate::update(Scene& scene, Undo& undo) {

	update_bvh(scene, undo);

	float dt = std::clamp(sim_timer.s(), 0.0f, 0.05f);

	step(scene, dt);
	sim_timer.reset();
}

void Simulate::build_collision(Scene& scene) {
	if (scene.particles.empty()) {
		collision = Scene::Collision();
	} else {
		collision = scene.build_collision(use_bvh, &thread_pool);
	}
}

void Simulate::clear_particles(Scene& scene) {
    for(auto& [name, particles] : scene.particles) {
        particles->reset();
	}
}

void Simulate::update_bvh(Scene& scene, Undo& undo) {
	if (cur_actions != undo.n_actions()) {
		build_collision(scene);
		cur_actions = undo.n_actions();
	}
}

void Simulate::ui_sidebar(Manager& manager, Scene& scene, Undo& undo, Widgets& widgets) {

    using namespace ImGui;

	update_bvh(scene, undo);

    Separator();
	Text("Simulation");

	if (Checkbox("Use BVH", &use_bvh)) {
		clear_particles(scene);
		build_collision(scene);
	}
}

} // namespace Gui
