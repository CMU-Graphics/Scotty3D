
#pragma once

#include "../lib/mathlib.h"
#include "../platform/gl.h"

#include "../pathtracer/pathtracer.h"
#include "../rasterizer/rasterizer.h"
#include "../scene/transform.h"

#include "../geometry/util.h"
#include "../util/hdr_image.h"
#include "../util/viewer.h"

class Undo;
struct Launch_Settings;

namespace Gui {

class Manager;
class Animate;
class Simulate;

enum class Mode : uint8_t { layout, model, render, rig, animate, simulate };

enum class Axis : uint8_t { X, Y, Z };

enum class Widget_Type : uint8_t { move, rotate, scale, bevel, extrude, inset, count };

enum class Widget_IDs : uint32_t {
	none,
	x_mov,
	y_mov,
	z_mov,
	xy_mov,
	yz_mov,
	xz_mov,
	x_rot,
	y_rot,
	z_rot,
	x_scl,
	y_scl,
	z_scl,
	xyz_scl,
	count
};
static const uint32_t n_Widget_IDs = static_cast<uint32_t>(Widget_IDs::count);

class Widgets {
public:
	Widgets();
	Widget_Type active = Widget_Type::move;

	void end_drag();
	Transform apply_action(const Transform& pose);
	void start_drag(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir);
	void drag_to(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir, bool scale_invert, float snap = 0.0f);

	bool select(uint32_t id);
	void render(const Mat4& view, Vec3 pos, float scl);

	bool want_drag();
	bool is_dragging();

	void action_button(Widget_Type type, const std::string& name, bool wrap = true);

	void change_rot(Mat4 xf, Vec3 pose, Vec3 x, Vec3 y, Vec3 z);

private:
	void generate_lines(Vec3 pos);
	bool to_axis(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3& hit);
	bool to_plane(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3 norm, Vec3& hit);
	bool to_axis3(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3& hit);
	// interface data
	Axis axis = Axis::X;
	Vec3 drag_start, drag_end;
	Vec2 bevel_start, bevel_end;
	bool dragging = false, drag_plane = false;
	bool start_dragging = false;
	bool univ_scl = false;
	// render data

	GL::Lines lines;
	struct Widget_Mesh {
		uint32_t id;
		Vec3 rot;
		Spectrum color;
		GL::Mesh mesh;
	};
	Widget_Mesh x_mov, y_mov, z_mov;
	Widget_Mesh xy_mov, yz_mov, xz_mov;
	Widget_Mesh x_rot, y_rot, z_rot;
	Widget_Mesh x_scl, z_scl, y_scl;
	Widget_Mesh xyz_scl;
};

class Widget_Transform {
public:
	Widget_Type ui(Widget_Type active, Undo& undo, std::weak_ptr<Transform> apply_to);

	void update(Undo& undo, std::weak_ptr<Transform> t);

	Transform cache;
};

class Widget_Camera {
public:
	void ui(Undo& undo, const std::string& name, std::weak_ptr<Camera> apply_to);

private:
	Camera cache;
	bool ar_activated = false, fov_activated = false, near_activated = false,
		 aperture_activated = false, focal_dist_activated = false, ray_depth_activated = false;
};

class Widget_Delta_Light {
public:
	void ui(const std::string& name, Manager& manager, Undo& undo,
	        std::weak_ptr<Delta_Light> apply_to);

private:
	Delta_Light old;
	bool color_activated = false, intensity_activated = false, inner_activated = false,
		 outer_activated = false;
};

class Widget_Environment_Light {
public:
	void ui(Manager& manager, Undo& undo, std::weak_ptr<Environment_Light> apply_to);

private:
	Environment_Light old;
};

class Widget_Material {
public:
	void ui(Manager& manager, Undo& undo, std::weak_ptr<Material> apply_to);

private:
	Material old;
	bool ior_activated = false;
};

class Widget_Shape {
public:
	void ui(const std::string& name, Undo& undo, std::weak_ptr<Shape> apply_to);

private:
	Shape old;
};

class Widget_Particles {
public:
	void ui(Undo& undo, std::weak_ptr<Particles> apply_to);

private:
	Particles old;
	bool gravity_activated = false, radius_activated = false, initial_velocity_activated = false,
		 spread_angle_activated = false, lifetime_activated = false, pps_activated = false,
		 step_size_activated = false;
};

class Widget_Texture {
public:
	void ui(const std::string& name, Manager& manager, Undo& undo, std::weak_ptr<Texture> apply_to);

private:
	Texture old;
	Spectrum color;
	// float scale = 1.0f; //TODO: unused?
	bool color_activated = false, scale_activated = false;
};

class Halfedge_Mesh_Controls {
public:
	//shows/executes mesh UI tasks, wrapping any actual modification in 'apply':
	void ui(std::function< void(std::function< void(Halfedge_Mesh &) > const &) > const &apply);
	std::optional<Halfedge_Mesh> create_shape();

private:
	float corner_normals_angle = 30.0f;
	Util::Shape type = Util::Shape::cube;
	float cube_radius = 1.0f;
	float square_radius = 1.0f;
	float pentagon_radius = 1.0f;
	float cylinder_radius = 0.5f, cylinder_height = 2.0f;
	uint32_t cylinder_sides = 12;
	float torus_inner_radius = 0.8f, torus_outer_radius = 1.0f;
	uint32_t torus_sides = 32, torus_rings = 16;
	float cone_bottom_radius = 1.0f, cone_top_radius = 0.1f, cone_height = 1.0f;
	uint32_t cone_sides = 12;
	float sphere_radius = 1.0f;
	uint32_t sphere_subdivisions = 2;
};

class Widget_Halfedge_Mesh {
public:
	Mode ui(Mode current, const std::string& name, Undo& undo, std::weak_ptr<Halfedge_Mesh> apply_to);
	void ui(const std::string& name, Undo& undo, std::weak_ptr<Halfedge_Mesh> apply_to);
private:
	Halfedge_Mesh_Controls controls;
};

class Widget_Skinned_Mesh {
public:
	Mode ui(Mode current, const std::string& name, Undo& undo, std::weak_ptr<Skinned_Mesh> apply_to);
	void ui(const std::string& name, Undo& undo, std::weak_ptr<Skinned_Mesh> apply_to);

private:
	Halfedge_Mesh_Controls controls;
};

class Widget_Render {
public:
	Widget_Render() = default;

	bool ui_render(Scene& scene, Manager& manager, Undo& undo, View_3D& gui_cam, std::string& err);
	void ui_animate(Scene& scene, Manager& manager, Undo& undo, View_3D& gui_cam, uint32_t max_frame);

	std::string step_animation(Scene& scene, Animator& animator, Manager& manager);

	void open(Scene &scene);
	void render_log(const Mat4& view);
	bool in_progress();

	PT::Pathtracer& tracer();

private:
	void begin_window(Scene& scene, Undo& undo, Manager& manager, View_3D& gui_cam);
	void display_output();

	enum class Method : uint8_t { hardware_raster, software_raster, path_trace, count };
	static const char* Method_Names[static_cast<uint8_t>(Method::count)];

	Method method = Method::path_trace;
	std::weak_ptr<Instance::Camera> render_cam;

	float exposure = 1.0f;
	bool use_bvh = true;
	bool has_rendered = false, rebuild_ray_log = false;
	bool render_window = false, render_window_focus = false;
	bool quit = false;

	bool rendering_animation = false;
	uint32_t next_frame = 0, max_frame = 0;

	float render_progress = 0.0f;
	std::mutex report_mut;
	std::string folder;
	char output_path_buffer[256] = {};

	GL::Lines ray_log;
	GL::Tex2D display;
	HDR_Image display_hdr;
	std::atomic<bool> update_display = false;
	GL::MSAA msaa;

	PT::Pathtracer pathtracer;
	std::unique_ptr< Rasterizer > rasterizer;
};

} // namespace Gui
