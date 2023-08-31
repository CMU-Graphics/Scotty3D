
#include <imgui/imgui_custom.h>
#include <tuple>

#include "../platform/renderer.h"
#include "../scene/undo.h"

#include "animate.h"
#include "manager.h"

namespace Gui {

Animate::Animate(::Gui::Manager& manager) : manager(manager) {
}

bool Animate::keydown(SDL_Keysym key) {

	if (key.sym == SDLK_SPACE) {
		playing = !playing;
		frame_timer.reset();
		return true;
	}

	return false;
}

bool Animate::render(Scene& scene, Widgets& widgets, Mat4 const &local_to_world, uint32_t next_id, View_3D& user_cam) {

	Mat4 view = user_cam.get_view();
	auto& R = Renderer::get();

	if (visualize_splines) {
		for (auto& e : spline_cache) {
			R.lines(e.second, view);
		}
	}

	auto mesh = skinned_mesh_select.lock();
	if (!mesh) return false;

	{ //render mesh:
		Renderer::SkeletonOpt opt(mesh->skeleton);
		opt.view = view * local_to_world;
		opt.posed = true;
		opt.face_mesh = nullptr;
		opt.first_id = next_id;
		opt.selected_base = selected_base;
		opt.selected_bone = selected_bone;
		opt.selected_handle = selected_handle;

		id_map = R.skeleton(opt);
	}

	if (selected_handle < mesh->skeleton.handles.size()) {
		//translate an IK handle:
		Skeleton::Handle &handle = mesh->skeleton.handles[selected_handle];

		Vec3 wpos = local_to_world * handle.target;
		float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);

		widgets.active = Widget_Type::move;
		widgets.render(view, wpos, scale);

		return true;

	} else if (selected_bone < mesh->skeleton.bones.size()) {
		//rotate a bone:
		std::vector< Mat4 > pose = mesh->skeleton.current_pose();

		Vec3 wpos = local_to_world * (pose[selected_bone] * Vec3{0.0f, 0.0f, 0.0f});
		float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);

		widgets.active = Widget_Type::rotate;
		// Change rotation axes to hopefully fix bone rotation
		Vec3 x,y,z;
		mesh->skeleton.bones[selected_bone].compute_rotation_axes(&x, &y, &z);
		Mat4 xf;
		if (mesh->skeleton.bones[selected_bone].parent < mesh->skeleton.bones.size()) {
			xf = pose[mesh->skeleton.bones[selected_bone].parent] * Mat4::translate(mesh->skeleton.bones[selected_bone].extent);
		} else {
			xf = Mat4::translate(mesh->skeleton.base + mesh->skeleton.base_offset);
		}

		widgets.change_rot(xf, mesh->skeleton.bones[selected_bone].pose, x, y, z);
		widgets.render(view, wpos, scale);

		return true;
	} else if (selected_base) {
		//translate the base offset:
		Vec3 wpos = local_to_world * (mesh->skeleton.base + mesh->skeleton.base_offset);
		float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);

		widgets.active = Widget_Type::move;
		widgets.render(view, wpos, scale);
		return true;
	}

	return false;
}

void Animate::make_spline(Animator& animator, const std::string& id) {

	auto path = Animator::Path{id, "translation"};
	auto spline = animator.splines.find(path);
	if (spline == animator.splines.end()) return;

	bool any = std::visit([](auto&& spline) { return spline.any(); }, spline->second);
	if (!any) return;

	auto entry = spline_cache.find(id);
	if (entry == spline_cache.end()) {
		std::tie(entry, std::ignore) = spline_cache.insert({id, GL::Lines()});
	}

	GL::Lines& lines = entry->second;
	lines.clear();

	Vec3 prev = animator.get<Vec3>(path, 0.0f).value();
	for (uint32_t i = 1; i < max_frame; i++) {

		float f = static_cast<float>(i);
		float c = static_cast<float>(i % 20) / 19.0f;
		Vec3 cur = animator.get<Vec3>(path, f).value();
		lines.add(prev, cur, Spectrum{c, c, 1.0f});
		prev = cur;
	}
}

void Animate::erase_mesh(const std::string& name) {
	if (name == mesh_name) {
		clear_select();
	}
}

void Animate::set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh) {
	clear_select();
	mesh_name = name;
	skinned_mesh_select = mesh;
}

void Animate::clear_select() {
	selected_bone = -1U;
	selected_handle = -1U;
	skinned_mesh_select.reset();
}

void Animate::ui_sidebar(Undo& undo, View_3D& user_cam) {
	using namespace ImGui;

	auto mesh = skinned_mesh_select.lock();

	if (!mesh) {
		clear_select();
	}

	if (mesh) {
		Checkbox("Solve IK", &run_solve_ik);
	}

	if (mesh && selected_handle < mesh->skeleton.handles.size()) {
		Skeleton::Handle &handle = mesh->skeleton.handles[selected_handle];

		Text("Edit IK Handle");
		DragFloat3("Pos", handle.target.data, 0.1f, 0.0f, 0.0f, "%.2f");
		if (IsItemActivated()) {
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivatedAfterEdit()
		&& (old_mesh.skeleton.handles.size() != mesh->skeleton.handles.size() || old_mesh.skeleton.handles[selected_handle].target != handle.target)) {
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
		if (Checkbox("Enable", &handle.enabled)) {
			bool new_enable = handle.enabled;
			handle.enabled = !new_enable;
			old_mesh = mesh->copy();
			handle.enabled = new_enable;
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
		Separator();

	} else if (mesh && selected_bone < mesh->skeleton.bones.size()) {
		Skeleton::Bone &bone = mesh->skeleton.bones[selected_bone];

		Text("Edit Joint");

		if (DragFloat3("Pose", bone.pose.data, 1.0f, 0.0f, 0.0f, "%.2f")) {
			dont_clear_select = true;
			manager.invalidate_gpu(mesh_name);
			dont_clear_select = false;
		}
		if (IsItemActivated()) {
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivatedAfterEdit()
		&& (old_mesh.skeleton.bones.size() != mesh->skeleton.bones.size() || old_mesh.skeleton.bones[selected_bone].pose != bone.pose)) {
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
		Separator();
	} else if (mesh && selected_base) {
		Text("Edit Base Point");
		if (DragFloat3("Offset", mesh->skeleton.base_offset.data, 0.01f, 0.0f, 0.0f, "%.2f")) {
			dont_clear_select = true;
			manager.invalidate_gpu(mesh_name);
			dont_clear_select = false;
		}
		if (IsItemActivated()) {
			old_mesh = mesh->copy();
		}
		if (IsItemDeactivatedAfterEdit() && old_mesh.skeleton.base_offset != mesh->skeleton.base_offset) {
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
	}

	if (mesh && run_solve_ik) {
		mesh->skeleton.solve_ik();
		dont_clear_select = true;
		manager.invalidate_gpu(mesh_name);
		dont_clear_select = false;
	}
}

bool Animate::skel_selected() {
	auto mesh = skinned_mesh_select.lock();
	return mesh && (selected_bone < mesh->skeleton.bones.size() || selected_handle < mesh->skeleton.handles.size() || selected_base);
}

void Animate::end_transform(Undo& undo) {
	auto mesh = skinned_mesh_select.lock();
	if (!mesh) return;

	if (selected_bone < mesh->skeleton.bones.size() || selected_handle < mesh->skeleton.handles.size() || selected_base) {
		dont_clear_select = true;
		undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
		dont_clear_select = false;
	}
}

Vec3 Animate::selected_pos(Mat4 const &local_to_world) {
	auto mesh = skinned_mesh_select.lock();
	if (!mesh) return local_to_world * Vec3{};

	if (selected_handle < mesh->skeleton.handles.size()) {
		Skeleton::Handle &handle = mesh->skeleton.handles[selected_handle];
		return local_to_world * handle.target;
	} else if (selected_bone < mesh->skeleton.bones.size()) {
		std::vector< Mat4 > pose = mesh->skeleton.current_pose();
		return local_to_world * (pose[selected_bone] * Vec3{});
	} else if (selected_base) {
		return local_to_world * (mesh->skeleton.base + mesh->skeleton.base_offset);
	}

	return local_to_world * Vec3{};
}

bool Animate::apply_transform(Widgets& widgets, Mat4 const &local_to_world) {
	auto mesh = skinned_mesh_select.lock();
	if (!mesh) return false;

	if (selected_handle < mesh->skeleton.handles.size()) {
		if (old_mesh.skeleton.handles.size() != mesh->skeleton.handles.size()) {
			warn("Somehow lost old mesh [handle count mis-match].");
			return true;
		}
		Skeleton::Handle const &old_handle = old_mesh.skeleton.handles[selected_handle];
		Skeleton::Handle &handle = mesh->skeleton.handles[selected_handle];

		//set up transform at the location of the old handle:
		Transform at;
		at.translation = local_to_world * old_handle.target;

		//move the new handle to the old handle:
		handle.target = local_to_world.inverse() * widgets.apply_action(at).translation;

		return true;
	} else if (selected_bone < mesh->skeleton.bones.size()) {
		if (old_mesh.skeleton.bones.size() != mesh->skeleton.bones.size()) {
			warn("Somehow lost old mesh [bone count mis-match].");
			return true;
		}

		Skeleton::Bone const &old_bone = old_mesh.skeleton.bones[selected_bone];
		Skeleton::Bone &bone = mesh->skeleton.bones[selected_bone];

		std::vector< Mat4 > old_pose = old_mesh.skeleton.current_pose();

		Transform at;
		at.translation = local_to_world * (old_pose[selected_bone] * Vec3(0.0f, 0.0f, 0.0f));
		//n.b. could set at.rotation to reflect local axes here but unclear that would help

		//rot is incremental rotation to apply in world space:
		Quat rot = widgets.apply_action(at).rotation;

		//rotation from bone's children -> world space:
		auto bone_to_world = local_to_world * old_pose[selected_bone];
		bone_to_world[0][3] = 0.0f;
		bone_to_world[1][3] = 0.0f;
		bone_to_world[2][3] = 0.0f;
		bone_to_world[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

		//rotation from world space to bone's parent:
		Mat4 world_to_parent;
		if (old_bone.parent < old_mesh.skeleton.bones.size()) {
			Mat4 parent_to_world = local_to_world * old_pose[old_bone.parent];
			world_to_parent = Mat4::transpose(parent_to_world);
		} else {
			world_to_parent = Mat4::transpose(local_to_world);
		}
		world_to_parent[0][3] = 0.0f;
		world_to_parent[1][3] = 0.0f;
		world_to_parent[2][3] = 0.0f;
		world_to_parent[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

		//thus, the new rotation we'd like for bone's children is:
		Mat4 new_rot = world_to_parent * rot.to_mat() * bone_to_world;

		Vec3 x, y, z;
		bone.compute_rotation_axes(&x, &y, &z);

		Mat4 bone_to_rotation_axes(Vec4(x, 0.0f), Vec4(y, 0.0f), Vec4(z, 0.0f),
		                           Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		auto rotation_axes_to_bone = Mat4::transpose(bone_to_rotation_axes);

		//convert into euler angles w.r.t. bone's local rotation axes:
		Mat4 new_rot_local =
		    rotation_axes_to_bone * new_rot * bone_to_rotation_axes;

		Vec3 new_euler = new_rot_local.to_euler();
		if(new_euler.valid()) {
			bone.pose = new_euler;
		}

		dont_clear_select = true;
		manager.invalidate_gpu(mesh_name);
		dont_clear_select = false;
		return true;
	} else if (selected_base) {
		//set up transform at the location of the old base offset:
		Transform at;
		at.translation = local_to_world * (old_mesh.skeleton.base + old_mesh.skeleton.base_offset);

		//update the base offset accordingly:
		mesh->skeleton.base_offset = (local_to_world.inverse() * widgets.apply_action(at).translation) - mesh->skeleton.base;
		return true;
	}

	return false;
}

bool Animate::select(Scene& scene, Widgets& widgets, Mat4 const &local_to_world, uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir) {
	auto mesh = skinned_mesh_select.lock();

	if (widgets.want_drag()) {

		if (mesh && selected_handle < mesh->skeleton.handles.size()) {
			//store old mesh:
			old_mesh = mesh->copy();

			//figure out where drag starts:
			Skeleton::Handle &handle = mesh->skeleton.handles[selected_handle];
			Vec3 base = local_to_world * handle.target;

			//start the drag:
			widgets.start_drag(base, cam, spos, dir);
		} else if (mesh && selected_bone < mesh->skeleton.bones.size()) {
			//store old mesh:
			old_mesh = mesh->copy();

			//figure out where drag starts:
			//Skeleton::Bone &bone = mesh->skeleton.bones[selected_bone]; //may eventually need this
			std::vector< Mat4 > pose = mesh->skeleton.current_pose();
			Vec3 base = local_to_world * pose[selected_bone] * Vec3{0.0f, 0.0f, 0.0f};

			//start the drag:
			widgets.start_drag(base, cam, spos, dir);
		} else if (mesh && selected_base) {
			//store old mesh:
			old_mesh = mesh->copy();

			//figure out where drag starts:
			Vec3 base = local_to_world * (mesh->skeleton.base + mesh->skeleton.base_offset);

			//start the drag:
			widgets.start_drag(base, cam, spos, dir);
		} else {
			widgets.start_drag(local_to_world * Vec3{}, cam, spos, dir);
		}
		return true;

	} else if (mesh) {
		if (id_map.bone_ids_begin <= id && id < id_map.bone_ids_end) {
			selected_bone = id - id_map.bone_ids_begin;
			selected_handle = -1U;
			selected_base = false;
			widgets.active = Widget_Type::rotate;
		} else if (id_map.handle_ids_begin <= id && id < id_map.handle_ids_end) {
			selected_bone = -1U;
			selected_handle = id - id_map.handle_ids_begin;
			selected_base = false;
			widgets.active = Widget_Type::move;
		} else if (id == id_map.base_id) {
			selected_bone = -1U;
			selected_handle = -1U;
			selected_base = true;
			widgets.active = Widget_Type::move;
		}
	} else {
		manager.set_select(id);
	}

	return false;
}

void Animate::ui_timeline(Undo& undo, Animator& animator, Scene& scene,
                          View_3D& gui_cam, std::optional<std::string> selected) {

	// NOTE(max): this is pretty messy
	//      Would be good to add the ability to set per-component keyframes
	//      ^ I started with that but it was hard to make work with assimp and
	//        generally made everything a lot messier.

	using namespace ImGui;

	ImVec2 size = GetWindowSize();

	Columns(2);
	SetColumnWidth(0, 150.0f);

	/*
	if (Button("<<##Reset")) {
		//TODO
	}
	*/

	if (!playing) {
		if (Button("Play")) {
			if (!ui_render.in_progress()) {
				playing = true;
				frame_timer.reset();
			}
		}
	} else {
		if (Button("Pause")) {
			playing = false;
		}
	}

	if (ui_render.in_progress()) playing = false;

	SameLine();
	if (Button("Render")) {
		ui_render.open(scene);
	}
	ui_render.ui_animate(scene, manager, undo, gui_cam, max_frame);

	PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
	Dummy({1.0f, 4.0f});
	PopStyleVar();

	if (Button("Add Frames")) {
		undo.anim_set_max_frame(*this, max_frame + uint32_t(animator.frame_rate), max_frame);
	}

	if (Button("Crop End")) {
		undo.anim_set_max_frame(*this, current_frame + 1, max_frame);
		jump_to_frame(scene, animator, static_cast<float>(current_frame));
	}

	DragFloat("Rate", &animator.frame_rate, 1.0f, 1.0f, 240.0f);
	animator.frame_rate = std::clamp(animator.frame_rate, 1.0f, 240.0f);

	Checkbox("Draw Splines", &visualize_splines);

	NextColumn();

	bool frame_changed = false;

	Text("Keyframe:");
	SameLine();

	auto all_keys = animator.all_keys();

	auto set_item = [&, this](const std::string& name) {
		undo.anim_set_keyframe(name, static_cast<float>(current_frame));
		make_spline(animator, name);
	};
	auto clear_item = [&, this](const std::string& name) {
		undo.anim_clear_keyframe(name, static_cast<float>(current_frame));
		make_spline(animator, name);
	};

	if (Button("Set") && selected) {
		set_item(selected.value());
	}

	SameLine();
	if (Button("Clear") && selected) {
		clear_item(selected.value());
	}

	SameLine();
	if (Button("Set All")) {
		size_t n = undo.n_actions();
		scene.for_each([&](const std::string& name, auto&&) { set_item(name); });
		undo.bundle_last(undo.n_actions() - n);
	}

	SameLine();
	if (Button("Clear All")) {
		size_t n = undo.n_actions();
		scene.for_each([&](const std::string& name, auto&&) { clear_item(name); });
		undo.bundle_last(undo.n_actions() - n);
	}

	SameLine();
	if (Button("Move Left") && current_frame > 0 && selected) {
		auto name = selected.value();
		if (animator.keys(name).count(static_cast<float>(current_frame))) {
			clear_item(name);
			current_frame--;
			set_item(name);
			undo.bundle_last(2);
		}
		frame_changed = true;
	}

	SameLine();
	if (Button("Move Right") && current_frame < max_frame - 1 && selected) {
		auto name = selected.value();
		if (animator.keys(name).count(static_cast<float>(current_frame))) {
			clear_item(name);
			current_frame++;
			set_item(name);
			undo.bundle_last(2);
		}
		frame_changed = true;
	}

	Separator();
	Dummy({74.0f, 1.0f});
	SameLine();
	if (SliderUInt32("Frame", &current_frame, 0, max_frame - 1)) {
		frame_changed = true;
	}
	BeginChild("Timeline", {size.x - 20.0f, size.y - 80.0f}, false,
	           ImGuiWindowFlags_HorizontalScrollbar);

	PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

	std::vector<bool> frames;
	std::vector<std::string> live_ids;

	scene.for_each([&, this](const std::string& name, auto&&) {
		if (!animator.has_channels(scene, name)) return;

		frames.clear();
		frames.resize(max_frame);

		ImVec2 size = CalcTextSize(name.c_str());
		if (selected && name == selected.value())
			TextColored({Color::outline.r, Color::outline.g, Color::outline.b, 1.0f}, "%s",
			            name.c_str());
		else
			Text("%s", name.c_str());
		SameLine();
		Dummy({80.0f - size.x, 1.0f});
		SameLine();

		PushID(name.c_str());

		auto keys = animator.keys(name);
		for (float f : keys) {
			uint32_t frame = static_cast<uint32_t>(std::round(f));
			if (frame < max_frame) frames[frame] = true;
		}

		for (uint32_t i = 0; i < max_frame; i++) {
			if (i > 0) SameLine();
			PushID(i);

			bool color = false;
			std::string label = "_";

			if (i == current_frame) {
				PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
				color = true;
			}

			if (frames[i]) {
				label = "*";
				if (i != current_frame) {
					color = true;
					PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonHovered));
				}
			}

			if (SmallButton(label.c_str())) {
				current_frame = i;
				frame_changed = true;
				manager.set_select(name);
			}
			if (color) PopStyleColor();
			PopID();
		}

		live_ids.push_back(name);

		SameLine();
		Dummy({142.0f, 1.0f});
		PopID();
	});

	PopStyleVar();
	EndChild();

	std::unordered_map<std::string, GL::Lines> new_cache;
	for (std::string i : live_ids) {
		auto entry = spline_cache.find(i);
		if (entry != spline_cache.end()) {
			new_cache[i] = std::move(entry->second);
		}
	}
	spline_cache = std::move(new_cache);

	if (frame_changed) jump_to_frame(scene, animator, float(current_frame));
}

void Animate::jump_to_frame(Scene& scene, Animator& animator, float frame) {
	uint32_t target_frame = uint32_t(std::max(0.0f, std::round(frame)));

	//TODO: could add user options to show particles perfectly in sync, in which case there might be (a lot!) of simulation here

	current_frame = target_frame;
	if (current_frame == 0) manager.get_simulate().clear_particles(scene);
	animator.drive(scene, frame);
	manager.get_simulate().build_collision(scene);
	for(auto& [name, _] : scene.skinned_meshes) {
		manager.invalidate_gpu(name);
	}
}

void Animate::set_max(uint32_t n_frames) {
	max_frame = std::max(n_frames, 1u);
	current_frame = std::min(current_frame, max_frame - 1);
}

uint32_t Animate::n_frames() const {
	return max_frame;
}

std::string Animate::pump_output(Scene& scene, Animator& animator) {
	return ui_render.step_animation(scene, animator, manager);
}

void Animate::refresh(Scene& scene, Animator& animator) {
	current_frame = 0;
	set_max(std::max(n_frames(), static_cast<uint32_t>(std::ceil(animator.max_key()))));
	jump_to_frame(scene, animator, static_cast<float>(current_frame));
	for (auto& spline : animator.splines) {
		make_spline(animator, (spline.first).first);
	}
}

void Animate::clear() {
	selected_bone = -1U;
	selected_handle = -1U;
	selected_base = false;
	skinned_mesh_select.reset();
	mesh_name = {};
}

void Animate::invalidate(const std::string& name) {
	spline_cache.erase(name);
	if (name == mesh_name) {
		if (!dont_clear_select) {
			selected_bone = -1U;
			selected_handle = -1U;
			selected_base = false;
		}
	}
}

bool Animate::playing_or_rendering() {
	return playing || ui_render.in_progress();
}

void Animate::update(Scene& scene, Animator& animator) {
	if (!playing) return;

	bool updated = false;

	if (frame_timer.s() > 1.0f / animator.frame_rate) {
		if (current_frame == max_frame - 1) {
			playing = false;
			current_frame = 0;
		} else {
			Scene::StepOpts opts;
			opts.use_bvh = manager.get_simulate().use_bvh;
			//opts.thread_pool = /* TODO */;
			if (current_frame == 0) opts.reset = true;
			scene.step(animator, float(current_frame), float(current_frame + 1), 1.0f / animator.frame_rate, opts);
			updated = true;

			current_frame++;
		}
		frame_timer.reset();
	}

	if (updated) {
		for(auto& [name, _] : scene.skinned_meshes) {
			manager.invalidate_gpu(name);
		}
	}
}

} // namespace Gui
