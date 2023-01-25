
#pragma once

#include "../platform/renderer.h"
#include "../util/timer.h"
#include "widgets.h"

#include <SDL_keyboard.h>
#include <set>

namespace Gui {

enum class Mode : uint8_t;
class Manager;

class Animate {
public:
	Animate(Manager& manager);

	bool keydown(SDL_Keysym key);

	Vec3 selected_pos(Mat4 const &local_to_world);
	void end_transform(Undo& undo);
	bool apply_transform(Widgets& widgets, Mat4 const &local_to_world);

	void clear_select();
	bool skel_selected();
	bool select(Scene& scene, Widgets& widgets, Mat4 const &local_to_world, uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir);
	void erase_mesh(const std::string& name);
	void set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh);

	bool render(Scene& scene, Widgets& widgets, Mat4 const &local_to_world, uint32_t next_id, View_3D& cam);
	void ui_timeline(Undo& undo, Animator& animator, Scene& scene, View_3D& gui_cam, std::optional<std::string> selected);
	void ui_sidebar(Undo& undo, View_3D& user_cam);

	bool playing_or_rendering();
	void clear();
	void update(Scene& scene, Animator& animator);
	void refresh(Scene& scene, Animator& animator);

	std::string pump_output(Scene& scene, Animator& animator);
	uint32_t n_frames() const;
	void set_max(uint32_t n_frames);
	void make_spline(Animator& animator, const std::string& id);

	void invalidate(const std::string& name);

private:
	bool playing = false;
	Timer frame_timer;

	void jump_to_frame(Scene& scene, Animator& animator, float frame);

	uint32_t current_frame = 0;
	uint32_t max_frame = 96;

	Widget_Render ui_render;
	Manager& manager;

	std::string mesh_name;
	std::weak_ptr<Skinned_Mesh> skinned_mesh_select;
	Skeleton::BoneIndex selected_bone = -1U;
	Skeleton::HandleIndex selected_handle = -1U;
	bool selected_base = false;
	bool run_solve_ik = true;

	Skinned_Mesh old_mesh; //stored when *any* edit starts -- used for updating undo stack

	Renderer::Skeleton_ID_Map id_map;

	bool visualize_splines = false;
	bool dont_clear_select = false;
	std::unordered_map<std::string, GL::Lines> spline_cache;
};

} // namespace Gui
