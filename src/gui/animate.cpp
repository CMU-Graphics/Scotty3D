
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

bool Animate::render(Scene& scene, Widgets& widgets, Mat4 pose, uint32_t next_id,
                     View_3D& user_cam) {

	Mat4 view = user_cam.get_view();
	auto& R = Renderer::get();

	if (visualize_splines) {
		for (auto& e : spline_cache) {
			R.lines(e.second, view);
		}
	}

	if (!skinned_mesh_select.expired()) {

		auto mesh = skinned_mesh_select.lock();

		Renderer::SkeletonOpt opt(mesh->skeleton);
		opt.view = view * pose;
		opt.posed = true;
		opt.face_mesh = nullptr;
		opt.base_id = next_id;
		opt.root_selected = false;
		opt.selected_bone = selected_bone;
		opt.selected_handle = selected_handle;

		std::tie(id_to_bone, id_to_handle) = R.skeleton(opt);

		if (!selected_handle.expired()) {
			widgets.active = Widget_Type::move;
		} else if (!selected_bone.expired()) {
			widgets.active = Widget_Type::rotate;
		}
	}

	if (!selected_handle.expired()) {

		auto mesh = skinned_mesh_select.lock();
		assert(mesh);

		auto handle = selected_handle.lock();

		Vec3 wpos = pose * (handle->target + mesh->skeleton.base);
		float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);
		widgets.render(view, wpos, scale);

		return true;

	} else if (!selected_bone.expired()) {

		auto mesh = skinned_mesh_select.lock();
		assert(mesh);

		auto bone = selected_bone.lock();

		Vec3 wpos = pose * mesh->skeleton.j_to_posed(bone) * Vec3{};
		float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);
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
		skinned_mesh_select.reset();
		selected_bone.reset();
		selected_handle.reset();
	}
}

void Animate::set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh) {
	mesh_name = name;
	skinned_mesh_select = mesh;
	selected_bone.reset();
	selected_handle.reset();
}

void Animate::clear_select() {
	selected_bone.reset();
	selected_handle.reset();
	skinned_mesh_select.reset();
}

void Animate::ui_sidebar(Undo& undo, View_3D& user_cam) {

	using namespace ImGui;

	if (skinned_mesh_select.expired()) {
		selected_bone.reset();
		selected_handle.reset();
	}

	if (!selected_handle.expired()) {

		auto mesh = skinned_mesh_select.lock();
		auto handle = selected_handle.lock();

		Text("Edit IK Handle");
		DragFloat3("Pos", handle->target.data, 0.1f, 0.0f, 0.0f, "%.2f");
		if (IsItemActivated()) old_pos = handle->target;
		if (IsItemDeactivatedAfterEdit() && old_pos != handle->target) {
			Vec3 new_pos = handle->target;
			handle->target = old_pos;
			old_mesh = mesh->copy();
			handle->target = new_pos;
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
		if (Checkbox("Enable", &handle->enabled)) {
			bool new_enable = handle->enabled;
			handle->enabled = !new_enable;
			old_mesh = mesh->copy();
			handle->enabled = new_enable;
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
		Separator();

	} else if (!selected_bone.expired()) {

		auto mesh = skinned_mesh_select.lock();
		auto bone = selected_bone.lock();

		Text("Edit Joint");

		if (DragFloat3("Pose", bone->pose.data, 1.0f, 0.0f, 0.0f, "%.2f")) {
			dont_clear_select = true;
			manager.invalidate_gpu(mesh_name);
			dont_clear_select = false;
		}

		if (IsItemActivated()) old_euler = bone->pose;
		if (IsItemDeactivatedAfterEdit() && old_euler != bone->pose) {
			bone->pose = bone->pose.range(0.0f, 360.0f);
			Vec3 new_euler = bone->pose;
			bone->pose = old_euler;
			old_mesh = mesh->copy();
			bone->pose = new_euler;
			dont_clear_select = true;
			undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
			dont_clear_select = false;
		}
		Separator();
	}

	if (!skinned_mesh_select.expired()) {
		auto mesh = skinned_mesh_select.lock();
		if (mesh->skeleton.run_ik(100)) {
			dont_clear_select = true;
			manager.invalidate_gpu(mesh_name);
			dont_clear_select = false;
		}
	}
}

bool Animate::skel_selected() {
	return !selected_bone.expired() || !selected_handle.expired();
}

void Animate::end_transform(Undo& undo) {

	if (skinned_mesh_select.expired()) return;

	if (!selected_bone.expired() || !selected_handle.expired()) {
		dont_clear_select = true;
		undo.update_cached<Skinned_Mesh>(mesh_name, skinned_mesh_select, std::move(old_mesh));
		dont_clear_select = false;
	}
}

Vec3 Animate::selected_pos(Mat4 pose) {

	auto mesh = skinned_mesh_select.lock();
	if (!mesh) return pose * Vec3{};

	if (!selected_handle.expired()) {
		auto handle = selected_handle.lock();
		return pose * (handle->target + mesh->skeleton.base);
	} else if (!selected_bone.expired()) {
		auto bone = selected_bone.lock();
		return pose * (mesh->skeleton.j_to_posed(bone) * Vec3{});
	}

	return pose * Vec3{};
}

bool Animate::apply_transform(Widgets& widgets) {

	Transform old_translate;
	old_translate.translation = old_pos;
	old_translate.rotation = Quat::euler(old_euler_spaghetti_code);

	if (!selected_handle.expired()) {

		auto mesh = skinned_mesh_select.lock();
		auto handle = selected_handle.lock();

		Vec3 p = old_T * widgets.apply_action(old_translate).translation;
		handle->target = p - mesh->skeleton.base;
		return true;

	} else if (!selected_bone.expired()) {

		auto mesh = skinned_mesh_select.lock();
		auto bone = selected_bone.lock();

		Quat rot = widgets.apply_action(old_translate).rotation;
		bone->pose = (old_T * rot.to_mat()).to_euler();

		dont_clear_select = true;
		manager.invalidate_gpu(mesh_name);
		dont_clear_select = false;
		return true;
	}

	return false;
}

bool Animate::select(Scene& scene, Widgets& widgets, Mat4 pose, uint32_t id,
                     Vec3 cam, Vec2 spos, Vec3 dir) {

	if (widgets.want_drag()) {

		if (!selected_handle.expired()) {

			auto handle = selected_handle.lock();
			auto mesh = skinned_mesh_select.lock();
			assert(mesh);

			Vec3 base = pose * (handle->target + mesh->skeleton.base);

			old_mesh = mesh->copy();
			old_pos = base;
			old_euler = handle->target;
			old_euler_spaghetti_code = Vec3{};

			widgets.start_drag(base, cam, spos, dir);
			old_T = pose.inverse();

		} else if (!selected_bone.expired()) {

			auto bone = selected_bone.lock();
			auto mesh = skinned_mesh_select.lock();
			assert(mesh);

			Vec3 base = pose * mesh->skeleton.j_to_posed(bone) * Vec3{};
			Mat4 j_to_p = pose * mesh->skeleton.j_to_posed(bone);
			old_mesh = mesh->copy();
			old_pos = Vec3{};
			old_euler = bone->pose;
			old_euler_spaghetti_code = j_to_p.to_euler();
			j_to_p = j_to_p * Mat4::euler(old_euler).inverse();
			old_T = j_to_p.inverse();

			widgets.start_drag(base, cam, spos, dir);

		} else {

			widgets.start_drag(pose * Vec3{}, cam, spos, dir);
		}
		return true;

	} else if (!skinned_mesh_select.expired()) {

		auto mesh = skinned_mesh_select.lock();

		selected_bone = id_to_bone[id];
		selected_handle = id_to_handle[id];
		if (!selected_bone.expired()) {
			widgets.active = Widget_Type::rotate;
		} else if (!selected_handle.expired()) {
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

	if (!playing) {
		if (Button("Play")) {
			if (!ui_render.in_progress()) {
				playing = true;
				frame_timer.reset();
			}
		}
	} else {
		if (Button("Stop")) {
			playing = false;
		}
	}

	if (ui_render.in_progress()) playing = false;

	SameLine();
	if (Button("Render")) {
		ui_render.open();
	}
	ui_render.ui_animate(scene, manager, undo, gui_cam, max_frame);

	PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
	Dummy({1.0f, 4.0f});
	PopStyleVar();

	if (Button("Add Frames")) {
		undo.anim_set_max_frame(*this, max_frame + frame_rate, max_frame);
	}

	if (Button("Crop End")) {
		undo.anim_set_max_frame(*this, current_frame + 1, max_frame);
		set_time(scene, animator, static_cast<float>(current_frame));
	}

	SliderUInt32("Rate", &frame_rate, 1, 240);
	frame_rate = std::clamp(frame_rate, 1u, 240u);

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

	if (frame_changed) update(scene, animator);
}

void Animate::step_sim(Scene& scene) {
	manager.get_simulate().build_scene(scene);
	manager.get_simulate().step(scene, 1.0f / frame_rate);
}

void Animate::set_time(Scene& scene, Animator& animator, float time) {
	current_frame = static_cast<uint32_t>(time);
	animator.drive(scene, time);
	manager.get_simulate().build_scene(scene);
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
	set_time(scene, animator, static_cast<float>(current_frame));
	for (auto& spline : animator.splines) {
		make_spline(animator, (spline.first).first);
	}
}

void Animate::clear() {
	selected_bone.reset();
	selected_handle.reset();
	skinned_mesh_select.reset();
	mesh_name = {};
}

void Animate::invalidate(const std::string& name) {
	spline_cache.erase(name);
	if (name == mesh_name) {
		if (!dont_clear_select) {
			selected_bone.reset();
			selected_handle.reset();
		}
	}
}

bool Animate::playing_or_rendering() {
	return playing || ui_render.in_progress();
}

void Animate::update(Scene& scene, Animator& animator) {

	if (playing) {

		if (frame_timer.s() > 1.0f / frame_rate) {

			if (current_frame == max_frame - 1) {
				playing = false;
				current_frame = 0;
			} else {
				current_frame++;
				step_sim(scene);
			}

			frame_timer.reset();
		}
	}

	if (displayed_frame != current_frame) {
		set_time(scene, animator, static_cast<float>(current_frame));
		displayed_frame = current_frame;
	}
}

} // namespace Gui
