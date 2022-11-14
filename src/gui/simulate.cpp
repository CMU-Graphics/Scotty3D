
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
			float radius = inst->mesh.expired() ? 1.0f : inst->mesh.lock()->radius();
        	inst->particles.lock()->advance(scene_obj, to_world, radius, dt);
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

void Simulate::build_scene(Scene& scene) {

	if (scene.particles.empty()) return;
    
	shapes.clear();
	meshes.clear();

    std::unordered_map<std::shared_ptr<Halfedge_Mesh>, std::string> mesh_names;
	std::unordered_map<std::shared_ptr<Skinned_Mesh>, std::string> skinned_mesh_names;
	std::unordered_map<std::shared_ptr<Shape>, std::string> shape_names;
	{
		std::vector<std::future<std::pair<std::string, PT::Tri_Mesh>>> mesh_futs;

		for (const auto& [name, mesh] : scene.meshes) {
			mesh_names[mesh] = name;
			mesh_futs.emplace_back(thread_pool.enqueue([name=name,mesh=mesh,this]() {
				return std::pair{name, PT::Tri_Mesh(Indexed_Mesh::from_halfedge_mesh(
										   *mesh, Indexed_Mesh::SplitEdges), use_bvh)};
			}));
		}

		for (const auto& [name, mesh] : scene.skinned_meshes) {
			skinned_mesh_names[mesh] = name;
			mesh_futs.emplace_back(thread_pool.enqueue([name=name,mesh=mesh,this]() {
				return std::pair{name, PT::Tri_Mesh(mesh->posed_mesh(), use_bvh)};
			}));
		}

		for (const auto& [name, shape] : scene.shapes) {
			shape_names[shape] = name;
			shapes.emplace(name, *shape);
		}

		for (auto& f : mesh_futs) {
			auto [name, mesh] = f.get();
			meshes.emplace(name, std::move(mesh));
		}
	}

	// create scene instances
	std::vector<PT::Instance> objects;

	for (const auto& [name, mesh_inst] : scene.instances.meshes) {

		if (!mesh_inst->settings.visible) continue;
		if (mesh_inst->mesh.expired()) continue;
		auto& mesh = meshes.at(mesh_names.at(mesh_inst->mesh.lock()));

		Mat4 T = mesh_inst->transform.expired() ? Mat4::I : mesh_inst->transform.lock()->local_to_world();

		objects.emplace_back(&mesh, nullptr, T);
	}

	for (const auto& [name, mesh_inst] : scene.instances.skinned_meshes) {

		if (!mesh_inst->settings.visible) continue;
		if (mesh_inst->mesh.expired()) continue;
		auto& mesh = meshes.at(skinned_mesh_names.at(mesh_inst->mesh.lock()));

		Mat4 T = mesh_inst->transform.expired() ? Mat4::I : mesh_inst->transform.lock()->local_to_world();

		objects.emplace_back(&mesh, nullptr, T);
	}

	for (const auto& [name, shape_inst] : scene.instances.shapes) {

		if (!shape_inst->settings.visible) continue;
		if (shape_inst->shape.expired()) continue;
		auto& shape = shapes.at(shape_names.at(shape_inst->shape.lock()));

		Mat4 T = shape_inst->transform.expired() ? Mat4::I : shape_inst->transform.lock()->local_to_world();

		objects.emplace_back(&shape, nullptr, T);
	}

	if (use_bvh) {
		scene_obj = PT::Aggregate(PT::BVH<PT::Instance>(std::move(objects)));
	} else {
		scene_obj = PT::Aggregate(PT::List<PT::Instance>(std::move(objects)));
	}
}

void Simulate::clear_particles(Scene& scene) {
    for(auto& [name, particles] : scene.particles) {
        particles->reset();
	}
}

void Simulate::update_bvh(Scene& scene, Undo& undo) {
	if (cur_actions != undo.n_actions()) {
		build_scene(scene);
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
		build_scene(scene);
	}
}

} // namespace Gui
