
#include <imgui/imgui_custom.h>

#include "../platform/renderer.h"
#include "../scene/undo.h"
#include "rig.h"

namespace Gui {

void Rig::erase_selected(Undo& undo) {
	auto mesh = my_mesh.lock();
	if (!mesh) return;

	if (selected_bone < mesh->skeleton.bones.size()) {
		auto old = mesh->copy();
		mesh->skeleton.erase_bone(selected_bone);
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old));
	} else if (selected_handle < mesh->skeleton.handles.size()) {
		auto old = mesh->copy();
		mesh->skeleton.erase_handle(selected_handle);
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old));
	}
	selected_bone = -1U;
	selected_handle = -1U;
}

void Rig::invalidate(const std::string& name) {
	if (name == mesh_name) {
		needs_rebuild = true;
		if (!dont_clear_select) {
			selected_bone = -1U;
			new_bone = -1U;
			selected_handle = -1U;
		}
	}
}

void Rig::erase_mesh(const std::string& name) {
	if (name == mesh_name) {
		my_mesh.reset();
		mesh_name = {};
		needs_rebuild = true;
		selected_bone = -1U;
		new_bone = -1U;
		selected_handle = -1U;
	}
}

void Rig::set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh) {
	if (mesh_name != name) needs_rebuild = true;
	mesh_name = name;
	my_mesh = mesh;
	//maybe don't need to do this:
	selected_bone = -1U;
	new_bone = -1U;
	selected_handle = -1U;
}

void Rig::render(Widgets& widgets, View_3D& cam) {
	auto mesh = my_mesh.lock();
	if (!mesh) return;

	Mat4 view = cam.get_view();

	rebuild();
	Renderer& R = Renderer::get();

	Renderer::SkeletonOpt opt(mesh->skeleton, gpu_mesh);
	opt.view = view;
	opt.posed = false;
	opt.selected_bone = selected_bone;
	opt.selected_handle = selected_handle;
	opt.selected_base = selected_base;
	opt.first_id = n_Widget_IDs;

	id_map = R.skeleton(opt);

	if ((selected_bone < mesh->skeleton.bones.size()) || (selected_handle < mesh->skeleton.handles.size()) || selected_base) {

		widgets.active = Widget_Type::move;
		Vec3 pos;

		if (selected_bone < mesh->skeleton.bones.size()) {
			std::vector< Mat4 > pose = mesh->skeleton.bind_pose();
			pos = pose[selected_bone] * mesh->skeleton.bones[selected_bone].extent;
		} else if (selected_handle < mesh->skeleton.handles.size()) {
			pos = mesh->skeleton.handles[selected_handle].target;
		} else if (selected_base) {
			pos = mesh->skeleton.base;
		}

		float scale = std::min((cam.pos() - pos).norm() / 5.5f, 10.0f);
		widgets.render(view, pos, scale);
	}
}

void Rig::end_transform(Widgets& widgets, Undo& undo) {
	auto mesh = my_mesh.lock();
	if (!mesh) return;

	if (selected_base || (selected_bone < mesh->skeleton.bones.size()) || (selected_handle < mesh->skeleton.handles.size())) {
		dont_clear_select = true;
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		dont_clear_select = false;
	}
}

void Rig::apply_transform(Widgets& widgets) {
	auto mesh = my_mesh.lock();
	if (!mesh) return;

	Transform old_translate;
	old_translate.translation = old_pos;

	if (selected_base) {
		mesh->skeleton.base = widgets.apply_action(old_translate).translation;
	} else if (selected_bone < mesh->skeleton.bones.size()) {
		if (old_mesh.skeleton.bones.size() != mesh->skeleton.bones.size()) {
			warn("Lost track of old bones somehow (had %u, now have %u).", uint32_t(old_mesh.skeleton.bones.size()), uint32_t(mesh->skeleton.bones.size()));
			return;
		}
		//move bone tip while preserving child bone tip positions:
		Vec3 delta = widgets.apply_action(old_translate).translation - old_pos;

		for (uint32_t b = 0; b < mesh->skeleton.bones.size(); ++b) {
			Skeleton::Bone &bone = mesh->skeleton.bones[b];
			Skeleton::Bone const &old_bone = old_mesh.skeleton.bones[b];
			if (b == selected_bone) {
				bone.extent = old_bone.extent + delta;
			} else if (bone.parent == selected_bone) {
				bone.extent = old_bone.extent - delta;
			}
		}
	} else if (selected_handle < mesh->skeleton.handles.size()) {
		Vec3 new_pos = widgets.apply_action(old_translate).translation;
		mesh->skeleton.handles[selected_handle].target = new_pos;
	}
}

Vec3 Rig::selected_pos() {
	auto mesh = my_mesh.lock();
	if (!mesh) return Vec3{};

	if (selected_base) {
		return mesh->skeleton.base;
	} else if (selected_bone < mesh->skeleton.bones.size()) {
		std::vector< Mat4 > pose = mesh->skeleton.bind_pose();
		return pose[selected_bone] * mesh->skeleton.bones[selected_bone].extent;
	} else if (selected_handle < mesh->skeleton.handles.size()) {
		return mesh->skeleton.handles[selected_handle].target;
	}

	assert(false);
	return Vec3{};
}

void Rig::select(Scene& scene, Widgets& widgets, Undo& undo, uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir) {
	auto mesh = my_mesh.lock();
	if (!mesh) return;

	if (creating_bone) {
		selected_bone = new_bone;
		new_bone = -1U;
		selected_handle = -1U;

		creating_bone = false;
		selected_base = false;
	} else if (widgets.want_drag()) {
		if (selected_base) {
			old_pos = mesh->skeleton.base;
			old_mesh = mesh->copy();
		} else if (selected_bone < mesh->skeleton.bones.size()) {
			Skeleton::Bone const &selected = mesh->skeleton.bones[selected_bone];

			std::vector< Mat4 > pose = mesh->skeleton.bind_pose();

			old_pos = pose[selected_bone] * selected.extent;
			old_base = pose[selected_bone] * Vec3(0.0f, 0.0f, 0.0f);
			old_ext = selected.extent;
			old_mesh = mesh->copy();
		} else if (selected_handle < mesh->skeleton.handles.size()) {
			old_pos = mesh->skeleton.handles[selected_handle].target;
			old_mesh = mesh->copy();
		}
		widgets.start_drag(old_pos, cam, spos, dir);

	} else if (id >= n_Widget_IDs) {
		selected_bone = -1U;
		new_bone = -1U;
		selected_handle = -1U;
		selected_base = false;

		if (id >= id_map.bone_ids_begin && id < id_map.bone_ids_end) {
			selected_bone = id - id_map.bone_ids_begin;
		} else if (id >= id_map.handle_ids_begin && id < id_map.handle_ids_end) {
			selected_handle = id - id_map.handle_ids_begin;
		} else if (id == id_map.base_id) {
			selected_base = true;
		}
	} else if (id == 0) {
		selected_bone = -1U;
		new_bone = -1U;
		selected_handle = -1U;
		selected_base = false;
	}
}

void Rig::clear_select() {
	selected_bone = -1U;
	selected_handle = -1U;
	selected_base = false;
	new_bone = -1U;
}

void Rig::rebuild() {
	if (!needs_rebuild) return;
	needs_rebuild = false;

	auto mesh = my_mesh.lock();
	if (!mesh) return;

	mesh_accel = PT::Tri_Mesh(mesh->bind_mesh(), use_bvh);
	gpu_mesh = mesh->bind_mesh().to_gl();
}

void Rig::hover(Vec3 cam, Vec2 spos, Vec3 dir) {
	auto mesh = my_mesh.lock();
	if (!mesh) return;

	rebuild();

	if (creating_bone) {
		assert(new_bone < mesh->skeleton.bones.size());

		Ray f(cam, dir);
		PT::Trace hit1 = mesh_accel.hit(f);
		if (!hit1.hit) return;

		Ray s(hit1.position + dir * EPS_F, dir);
		PT::Trace hit2 = mesh_accel.hit(s);

		Vec3 pos = hit1.position;
		if (hit2.hit) pos = 0.5f * (hit1.position + hit2.position);

		mesh->skeleton.bones.at(new_bone).extent = pos - old_base;
		//my_mesh.lock()->bone_cache.clear();
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

	//re-get mesh in case it is out of sync with its name:
	my_mesh = scene.get<Skinned_Mesh>(mesh_name);
	auto mesh = my_mesh.lock();

	if (!mesh) {
		my_mesh.reset();
		mesh_name = {};
		selected_bone = -1U;
		new_bone = -1U;
		selected_handle = -1U;
		selected_base = false;
		creating_bone = false;
		needs_rebuild = true;
		return;
	}

	Separator();
	Text("Edit Mesh");
	if (Button("Recompute Weights")) {
		dont_clear_select = true;
		old_mesh = mesh->copy();
		mesh->skeleton.assign_bone_weights(&mesh->mesh);
		undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		dont_clear_select = false;
	}

	edit_widget.ui(mesh_name, undo, my_mesh);

	Separator();
	Text("Edit Skeleton");

	if (creating_bone) {
		if (Button("Cancel")) {
			creating_bone = false;
			mesh->skeleton.erase_bone(new_bone);
			new_bone = -1U;
			selected_handle = -1U;
		}
	} else {
		if (Button("New Bone")) {

			dont_clear_select = true;
			creating_bone = true;
			selected_handle = -1U;

			if ((selected_bone >= mesh->skeleton.bones.size()) || selected_base) {
				old_mesh = mesh->copy();
				old_base = mesh->skeleton.base;
				new_bone = mesh->skeleton.add_bone(-1U, Vec3{0.0f});
				undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
			} else {
				std::vector< Mat4 > pose = mesh->skeleton.bind_pose();
				old_mesh = mesh->copy();
				old_base = pose[selected_bone] * mesh->skeleton.bones[selected_bone].extent;
				new_bone = mesh->skeleton.add_bone(selected_bone, Vec3{0.0f});
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
	if (selected_bone < mesh->skeleton.bones.size()) {

		Skeleton::Bone &bone = mesh->skeleton.bones[selected_bone];

		Separator();
		Text("Edit Bone");

		DragFloat3("Extent", bone.extent.data, 0.1f);
		if (IsItemActivated()) {
			old_ext = bone.extent;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_ext != bone.extent) {
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		DragFloat("Roll", &bone.roll, 1.0f);
		bone.roll = bone.roll - 360.0f * std::round(bone.roll / 360.0f);
		if (IsItemActivated()) {
			old_roll = bone.roll;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_roll != bone.roll) {
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		DragFloat("Radius", &bone.radius, 0.01f, 0.0f, std::numeric_limits<float>::infinity());
		if (IsItemActivated()) {
			old_radius = bone.radius;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_radius != bone.radius) {
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		DragFloat3("Pose", bone.pose.data, 0.1f);
		if (IsItemActivated()) {
			old_pos = bone.pose;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_pos != bone.pose) {
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		if (Button("Add IK")) {
			std::vector< Mat4 > pose = mesh->skeleton.bind_pose();
			Vec3 tip = pose[selected_bone] * bone.extent;
			old_mesh = mesh->copy();
			uint32_t new_handle = mesh->skeleton.add_handle(selected_bone, tip);
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

			clear_select();
			selected_handle = new_handle;
		}
		SameLine();
		if (Button("Delete [del]")) {
			old_mesh = mesh->copy();
			mesh->skeleton.erase_bone(selected_bone);
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

			clear_select();
		}

	} else if (selected_handle < mesh->skeleton.handles.size()) {
		Skeleton::Handle &handle = mesh->skeleton.handles[selected_handle];

		Separator();
		Text("Edit Handle");

		DragFloat3("Target", handle.target.data, 0.1f);
		if (IsItemActivated()) {
			old_pos = handle.target;
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivated() && old_pos != handle.target) {
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		bool old_enable = handle.enabled;
		if (Checkbox("Enable", &handle.enabled)) {
			bool new_enable = handle.enabled;
			handle.enabled = old_enable;
			old_mesh = mesh->copy();
			handle.enabled = new_enable;
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));
		}

		if (Button("Delete [del]")) {
			old_mesh = mesh->copy();
			mesh->skeleton.erase_handle(selected_handle);
			undo.update_cached<Skinned_Mesh>(mesh_name, my_mesh, std::move(old_mesh));

			clear_select();
		}
	}
	dont_clear_select = false;

	return;
}

} // namespace Gui
