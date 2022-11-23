
#pragma once

#include <functional>

#include <SDL.h>
#include <imgui/imgui.h>

#include "../lib/mathlib.h"
#include "../scene/animator.h"
#include "../scene/scene.h"
#include "../util/viewer.h"

#include "modifiers.h"
#include "widgets.h"
#include "model.h"
#include "render.h"
#include "rig.h"
#include "simulate.h"
#include "animate.h"

class Undo;

namespace Gui {

#define RGBv(n, r, g, b) static inline const Spectrum n = Spectrum(r##.0f, g##.0f, b##.0f) / 255.0f;
struct Color {
	RGBv(black, 0, 0, 0);
	RGBv(white, 255, 255, 255);
	RGBv(outline, 240, 160, 70);
	RGBv(active, 242, 200, 70);
	RGBv(selected, 200, 125, 41);
	RGBv(hover, 102, 102, 204);
	RGBv(baseplane, 71, 71, 71);
	RGBv(background, 58, 58, 58);
	RGBv(red, 163, 66, 81);
	RGBv(green, 124, 172, 40);
	RGBv(blue, 64, 127, 193);
	RGBv(yellow, 238, 221, 79);
	RGBv(hoverg, 102, 204, 102);
	static Spectrum axis(Axis a);
};
#undef RGBv

class Manager {
public:
	Manager(Scene& scene, Undo& undo, Animator& animator, Vec2 window_dim);

	void set_error(std::string msg);
	void invalidate_gpu(const std::string& name);
	void update_dim(Vec2 dim);

	// Keyboard interactions
	bool keydown(SDL_Keysym key, View_3D& gui_cam);
	bool quit();

	// Mouse interactions
	bool select(uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods);
	void hover(uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods);
	void drag_to(Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods);
	void end_drag();
	
	void clear_select();
	void set_select(const std::string& name);
	void set_select(uint32_t id);

	// Render the GUI
	void render_3d(View_3D& gui_cam);
	void render_ui(View_3D& gui_cam);

	void render_image(const std::string& tex_name, Vec2 size);
	std::weak_ptr<Transform> render_instances(Mat4 view, bool gui = true);

	void edit_texture(std::weak_ptr<Texture> tex);
	std::optional<std::string> choose_image();

	template<typename I, typename R>
	bool choose_instance(const std::string& label, const std::shared_ptr<I>& instance, std::weak_ptr<R>& resource, bool can_be_null = false);
	void edit_camera(std::shared_ptr<Instance::Camera> inst, View_3D& gui_cam);

	Render& get_render();
	Animate& get_animate();
	Simulate& get_simulate();
	void shutdown();

private:
	void ui_error();
	void ui_settings();
	void ui_savefirst();
	void ui_scene_graph();
	void ui_properties(View_3D& gui_cam);
	void ui_new_object();
	void ui_resource_list();
	void ui_resource_list_physics();
	void ui_sidebar(float menu_height, View_3D& gui_cam);
	float ui_menu();

	void frame(View_3D& gui_cam);

	static inline const char* scene_file_types = "js3d;s3d";
	static inline const char* image_file_types = "png;jpg;exr;hdr;hdri;jpeg;tga;bmp;psd;gif";

public:
	enum class Load : uint8_t { new_scene, append };
	void new_scene();
	//if path is passed, doesn't show dialog:
	void load_scene(std::string *path = nullptr, Load strategy = Load::new_scene);
	//if path is passed, doesn't show dialog:
	bool save_scene_as(std::string *path = nullptr);

private:
	void erase_selected();
	void to_s3d();

	// Scene data
	Scene& scene;
	Undo& undo;
	Animator& animator;

	// UI state
	Mode mode = Mode::layout;
	GL::MSAA samples;
	bool error_shown = false, debug_shown = false, settings_shown = false;
	bool save_first_shown = false, already_denied_save = false, new_object_shown = false,
		 new_object_focus = false;
	std::string error_msg;
	std::string save_file;

	size_t n_actions_at_last_save = 0;
	std::function<void(bool)> after_save;
	Vec2 window_dim;
	
	// UI modes
	Model model;
	Render render;
	Rig rig;
	Simulate simulate;
	Animate animate;

	// 2D UI components
	Widget_Transform transform_widget;
	Widget_Camera camera_widget;
	Widget_Delta_Light delta_light_widget;
	Widget_Environment_Light environment_light_widget;
	Widget_Material material_widget;
	Widget_Shape shape_widget;
	Widget_Particles particles_widget;
	Widget_Texture texture_widget;
	Widget_Halfedge_Mesh halfedge_mesh_widget;
	Widget_Skinned_Mesh skinned_mesh_widget;

	// 3D UI components
	Widgets widgets;
	GL::Lines baseplane;
	GL::Mesh point_light_mesh, directional_light_mesh, spot_light_origin_mesh, particle_system_mesh;

	// Updated every frame; any meshes not in the cache will be added.
	// Meshes must be manually invalidated when they change.
	std::unordered_map<std::string, GL::Mesh> gpu_mesh_cache;
	std::unordered_map<std::string, GL::Lines> gpu_lines_cache;
	std::unordered_map<std::string, GL::Tex2D> gpu_texture_cache;
	void update_gpu_caches();

	// Instance to ID mapping for selection. Built every frame.
	std::weak_ptr<Transform> selected_instance_transform;
	std::optional<std::string> selected_object_name;
	std::unordered_map<uint32_t, std::string> id_to_instance;
	uint32_t next_id = 0;
};

}; // namespace Gui
