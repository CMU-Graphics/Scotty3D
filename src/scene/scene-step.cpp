#include "scene.h"
#include "animator.h"

#include "../util/thread_pool.h"

void Scene::step(Animator const &animator, float animate_from, float animate_to, float simulate_for, StepOpts const &opts) {

	//do simulation only if opts.simulate is true *and* some simulations actually exist in the scene:
	bool simulate = false;
	if (opts.simulate) {
		for(auto& [_, inst] : instances.particles) {
			auto parts = inst->particles.lock();
			if (parts) simulate = true;
		}
	}

	//if 'reset', drive scene to start time and reset simulations:
	if (opts.reset) {
		animator.drive(*this, animate_from); //drive animation to start time
		for(auto& [_, inst] : instances.particles) {
			auto parts = inst->particles.lock();
			if (parts) parts->reset();
		}
	}

	//tick simulations forward:
	if (simulate) {
		//build bvh/list of scene geometry:
		Collision collision = build_collision(opts.use_bvh, opts.thread_pool);

		//tick simulations:
		for(auto& [_, inst] : instances.particles) {
			auto parts = inst->particles.lock();
			if (!parts) continue;
			Mat4 to_world = inst->transform.expired() ? Mat4::I : inst->transform.lock()->local_to_world();
			parts->advance(collision.world, to_world, simulate_for);
		}
	}

	//drive animation to the ending time:
	if (opts.animate) animator.drive(*this, animate_to);

}

Scene::Collision Scene::build_collision(bool use_bvh, Thread_Pool *thread_pool) const {
	Collision collision;

	//first, convert all meshes -> PT::Tri_Mesh
	if (thread_pool) {
		std::vector<std::future<std::pair<Halfedge_Mesh const *, PT::Tri_Mesh>>> mesh_futs;

		for (const auto& [name, mesh] : meshes) {
			mesh_futs.emplace_back(thread_pool->enqueue([name=name,mesh=mesh,use_bvh]() {
				return std::pair{const_cast< const Halfedge_Mesh * >(mesh.get()), PT::Tri_Mesh(Indexed_Mesh::from_halfedge_mesh(*mesh, Indexed_Mesh::SplitEdges), use_bvh)};
			}));
		}

		for (const auto& [name, mesh] : skinned_meshes) {
			mesh_futs.emplace_back(thread_pool->enqueue([name=name,mesh=mesh,use_bvh]() {
				return std::pair{const_cast< const Halfedge_Mesh * >(&mesh->mesh), PT::Tri_Mesh(mesh->posed_mesh(), use_bvh)};
			}));
		}

		for (auto& f : mesh_futs) {
			auto [ptr, mesh] = f.get();
			collision.meshes.emplace(ptr, std::move(mesh));
		}
	} else {
		for (const auto& [name, mesh] : meshes) {
			collision.meshes.emplace( mesh.get(),
				PT::Tri_Mesh(Indexed_Mesh::from_halfedge_mesh(*mesh, Indexed_Mesh::SplitEdges), use_bvh)
			);
		}
		for (const auto& [name, mesh] : skinned_meshes) {
			collision.meshes.emplace( &mesh->mesh,
				PT::Tri_Mesh(mesh->posed_mesh(), use_bvh)
			);
		}
	}

	//now create instances of meshes/shapes:
	std::vector<PT::Instance> objects;

	for (const auto& [name, mesh_inst] : instances.meshes) {
		if (!mesh_inst->settings.collides) continue;
		auto mesh = mesh_inst->mesh.lock();
		if (!mesh) continue;
		auto const &pt_mesh = collision.meshes.at(mesh.get());

		Mat4 T = mesh_inst->transform.expired() ? Mat4::I : mesh_inst->transform.lock()->local_to_world();

		objects.emplace_back(&pt_mesh, nullptr, T);
	}

	for (const auto& [name, mesh_inst] : instances.skinned_meshes) {
		if (!mesh_inst->settings.collides) continue;
		auto mesh = mesh_inst->mesh.lock();
		if (!mesh) continue;
		auto const& pt_mesh = collision.meshes.at(&mesh->mesh);

		Mat4 T = mesh_inst->transform.expired() ? Mat4::I : mesh_inst->transform.lock()->local_to_world();

		objects.emplace_back(&pt_mesh, nullptr, T);
	}

	for (const auto& [name, shape_inst] : instances.shapes) {
		if (!shape_inst->settings.collides) continue;
		auto shape = shape_inst->shape.lock();
		if (!shape) continue;

		Mat4 T = shape_inst->transform.expired() ? Mat4::I : shape_inst->transform.lock()->local_to_world();

		objects.emplace_back(shape.get(), nullptr, T);
	}

	if (use_bvh) {
		collision.world = PT::Aggregate(PT::BVH<PT::Instance>(std::move(objects)));
	} else {
		collision.world = PT::Aggregate(PT::List<PT::Instance>(std::move(objects)));
	}

	return collision;
}
