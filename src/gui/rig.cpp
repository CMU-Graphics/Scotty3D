
#include <imgui/imgui_custom.h>

#include "../platform/renderer.h"
#include "../scene/undo.h"
#include "rig.h"

namespace Gui {

void Rig::erase_selected(Undo& undo) {
	if (my_mesh.expired()) return;
	auto mesh = my_mesh.lock();

	if (!selected_bone.expired()) {	
		auto old = mesh->copy();
		mesh->skeleton.erase(selected_bone);
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old));
		selected_bone.reset();
	} else if (!selected_handle.expired()) {
		auto old = mesh->copy();
		mesh->skeleton.erase(selected_handle);
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old));
		selected_handle.reset();
	}
}

void Rig::invalidate(const std::string& name) {
	if (name == mesh_name) {
		needs_rebuild = true;
		if (!dont_clear_select) {
			selected_bone.reset();
			selected_handle.reset();
			new_bone.reset();
		}
	}
}

void Rig::erase_mesh(const std::string& name) {
	if (name == mesh_name) {
		my_mesh.reset();
		mesh_name = {};
		needs_rebuild = true;
		selected_bone.reset();
		selected_handle.reset();
		new_bone.reset();
	}
}

void Rig::set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh) {
	if (mesh_name != name) needs_rebuild = true;
	mesh_name = name;
	my_mesh = mesh;
}

void Rig::render(Widgets& widgets, View_3D& cam) {

	if (my_mesh.expired()) return;
	auto mesh = my_mesh.lock();

	Mat4 view = cam.get_view();

	rebuild();
	Renderer& R = Renderer::get();

	Renderer::SkeletonOpt opt(mesh->skeleton, gpu_mesh);
	opt.view = view;
	opt.posed = false;
	opt.selected_bone = selected_bone;
	opt.selected_handle = selected_handle;
	opt.root_selected = root_selected;
	opt.base_id = n_Widget_IDs;

	std::tie(id_to_bone, id_to_handle) = R.skeleton(opt);

	if (!selected_bone.expired() || !selected_handle.expired() || root_selected) {

		widgets.active = Widget_Type::move;
		Vec3 pos;

		if (!selected_bone.expired())
			pos = mesh->skeleton.end_of(selected_bone);
		else if (!selected_handle.expired())
			pos = selected_handle.lock()->target + mesh->skeleton.base;
		else if (root_selected)
			pos = mesh->skeleton.base;

		float scale = std::min((cam.pos() - pos).norm() / 5.5f, 10.0f);
		widgets.render(view, pos, scale);
	}
}

void Rig::end_transform(Widgets& widgets, Undo& undo) {

	if (my_mesh.expired()) return;

	if (root_selected || !selected_bone.expired() || !selected_handle.expired()) {
		dont_clear_select = true;
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		dont_clear_select = false;

		my_mesh.lock()->bone_cache.clear();
	}
}

void Rig::apply_transform(Widgets& widgets) {

	if (my_mesh.expired()) return;
	auto mesh = my_mesh.lock();

	Transform old_translate;
	old_translate.translation = old_pos;

	if (root_selected) {
		mesh->skeleton.base = widgets.apply_action(old_translate).translation;
		mesh->bone_cache.clear();
	} else if (!selected_bone.expired()) {
		Vec3 new_pos = widgets.apply_action(old_translate).translation;
		selected_bone.lock()->extent = new_pos - old_base;
		mesh->bone_cache.clear();
	} else if (!selected_handle.expired()) {
		Vec3 new_pos = widgets.apply_action(old_translate).translation;
		selected_handle.lock()->target = new_pos - mesh->skeleton.base;
		mesh->bone_cache.clear();
	}
}

Vec3 Rig::selected_pos() {

	if (my_mesh.expired()) return Vec3{};
	auto mesh = my_mesh.lock();

	if (root_selected) {
		return mesh->skeleton.base;
	} else if (!selected_bone.expired()) {
		return mesh->skeleton.end_of(selected_bone);
	} else if (!selected_handle.expired()) {
		return selected_handle.lock()->target + mesh->skeleton.base;
	}

	assert(false);
	return Vec3{};
}

void Rig::select(Scene& scene, Widgets& widgets, Undo& undo, uint32_t id, Vec3 cam, Vec2 spos,
                 Vec3 dir) {

	if (my_mesh.expired()) return;
	auto mesh = my_mesh.lock();

	if (creating_bone) {

		selected_bone = new_bone;
		new_bone.reset();
		selected_handle.reset();

		creating_bone = false;
		root_selected = false;

	} else if (widgets.want_drag()) {

		if (root_selected) {
			old_pos = mesh->skeleton.base;
			old_mesh = mesh->copy();
		} else if (!selected_bone.expired()) {
			auto selected = selected_bone.lock();
			old_pos = mesh->skeleton.end_of(selected);
			old_base = selected->parent.expired() ? mesh->skeleton.base
			                                      : mesh->skeleton.end_of(selected->parent);
			old_ext = selected->extent;
			old_mesh = mesh->copy();
		} else if (!selected_handle.expired()) {
			old_pos = selected_handle.lock()->target + mesh->skeleton.base;
			old_mesh = mesh->copy();
		}
		widgets.start_drag(old_pos, cam, spos, dir);

	} else if (id >= n_Widget_IDs) {

		selected_bone = id_to_bone[id];
		selected_handle = id_to_handle[id];
		root_selected = id == n_Widget_IDs;
	
	} else if(id == 0) {

		selected_bone.reset();
		selected_handle.reset();
		root_selected = false;
	}
}

void Rig::clear_select() {
	selected_bone.reset();
	selected_handle.reset();
}

void Rig::rebuild() {
	if (my_mesh.expired()) return;
	auto mesh = my_mesh.lock();
	if (needs_rebuild) {
		needs_rebuild = false;
		mesh_accel = PT::Tri_Mesh(mesh->bind_mesh(), use_bvh);
		gpu_mesh = mesh->bind_mesh().to_gl();
	}
}

void Rig::hover(Vec3 cam, Vec2 spos, Vec3 dir) {

	if (my_mesh.expired()) return;

	rebuild();

	if (creating_bone) {
		assert(!new_bone.expired());

		Ray f(cam, dir);
		PT::Trace hit1 = mesh_accel.hit(f);
		if (!hit1.hit) return;

		Ray s(hit1.position + dir * EPS_F, dir);
		PT::Trace hit2 = mesh_accel.hit(s);

		Vec3 pos = hit1.position;
		if (hit2.hit) pos = 0.5f * (hit1.position + hit2.position);

		new_bone.lock()->extent = pos - old_base;
		my_mesh.lock()->bone_cache.clear();
	}
}

void Rig::ui_sidebar(Scene& scene, Undo& undo, Widgets& widgets) {

	using namespace ImGui;

	auto bullet = [&](const std::string& name, std::shared_ptr<Skinned_Mesh>& mesh) {
		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Bullet |
		                                ImGuiTreeNodeFlags_NoTreePushOnOpen |
		                                ImGuiTreeNodeFlags_SpanAvailWidth;
		bool is_selected = name == mesh_name;
		if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

		TreeNodeEx(name.c_str(), node_flags, "%s", name.c_str());
		if (IsItemClicked()) {
			mesh_name = name;
			my_mesh = mesh;
			needs_rebuild = true;
		}
	};
	if (CollapsingHeader("Skinned Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (auto& [name, _mesh] : scene.skinned_meshes) {
			bullet(name, _mesh);
		}
	}

	if(scene.get<Skinned_Mesh>(mesh_name).expired()) {
		my_mesh.reset();
		mesh_name = {};
		selected_bone.reset();
		selected_handle.reset();
		root_selected = false;
		needs_rebuild = true;
		return;
	}

	if (my_mesh.expired()) return;
	auto mesh = my_mesh.lock();

	Separator();
	Text("Edit Mesh");
	edit_widget.ui(mesh_name, undo, my_mesh);

	Separator();
	Text("Edit Skeleton");

	if (creating_bone) {
		if (Button("Cancel")) {
			creating_bone = false;
			mesh->skeleton.erase(new_bone);
			new_bone.reset();
			selected_handle.reset();
		}
	} else {
		if (Button("New Bone")) {

			dont_clear_select = true;
			creating_bone = true;
			selected_handle.reset();

			if (selected_bone.expired() || root_selected) {
				old_mesh = mesh->copy();
				new_bone = mesh->skeleton.add_root(Vec3{0.0f});
				old_base = mesh->skeleton.base;
				undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
			} else {
				old_mesh = mesh->copy();
				new_bone = selected_bone.lock()->add_child(Vec3{0.0f});
				old_base = mesh->skeleton.end_of(selected_bone);
				undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
			}

			dont_clear_select = false;
		}

		SameLine();

		if (Checkbox("Use BVH", &use_bvh)) {
			mesh_accel = PT::Tri_Mesh(mesh->bind_mesh(), use_bvh);
		}
	}

	dont_clear_select = true;
	if (!selected_bone.expired()) {

		auto bone = selected_bone.lock();

		Separator();
		Text("Edit Bone");

		bool reskin = false;

		reskin |= DragFloat3("Extent", bone->extent.data, 0.1f);
		if (IsItemActivated()) {
			old_ext = bone->extent;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_ext != bone->extent)
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

		reskin |=
			DragFloat("Radius", &bone->radius, 0.01f, 0.0f, std::numeric_limits<float>::infinity());
		if (IsItemActivated()) {
			old_r = bone->radius;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_r != bone->radius)
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

		DragFloat3("Pose", bone->pose.data, 0.1f);
		if (IsItemActivated()) {
			old_pos = bone->pose;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_pos != bone->pose)
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

		if (reskin) {
			mesh->bone_cache.clear();
		}

		if (Button("Add IK")) {
			old_mesh = mesh->copy();
			selected_handle = mesh->skeleton.add_handle(bone, mesh->skeleton.end_of(bone) - mesh->skeleton.base);
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
			selected_bone.reset();
		}
		SameLine();
		if (Button("Delete [del]")) {
			old_mesh = mesh->copy();
			mesh->skeleton.erase(selected_bone);
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
			selected_bone.reset();
			mesh->bone_cache.clear();
		}

	} else if (auto handle = selected_handle.lock()) {
		Separator();
		Text("Edit Handle");

		DragFloat3("Target", handle->target.data, 0.1f);
		if (IsItemActivated()) {
			old_pos = handle->target;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_pos != handle->target)
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

		bool old_enable = handle->enabled;
		if (Checkbox("Enable", &handle->enabled)) {
			bool new_enable = handle->enabled;
			handle->enabled = old_enable;
			old_mesh = mesh->copy();
			handle->enabled = new_enable;
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		if (Button("Delete [del]")) {
			old_mesh = mesh->copy();
			mesh->skeleton.erase(selected_handle);
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
			selected_handle.reset();
		}
	}
	dont_clear_select = false;

	return;
}

} // namespace Gui
