
#include <imgui/imgui_custom.h>
#include <iomanip>
#include <iostream>
#include <nfd/nfd.h>
#include <sf_libs/stb_image_write.h>
#include <sstream>

#include "manager.h"
#include "widgets.h"

#include "../app.h"
#include "../geometry/util.h"
#include "../platform/platform.h"
#include "../platform/renderer.h"
#include "../rasterizer/rasterizer.h"
#include "../pathtracer/aperture_shape.h"

static bool postfix(const std::string& path, const std::string& type) {
	if (path.length() >= type.length())
		return path.compare(path.length() - type.length(), type.length(), type) == 0;
	return false;
}

namespace Gui {

const char* Widget_Render::Method_Names[] = {"Hardware Rasterize", "Software Rasterize",
                                             "Path Trace"};

Widgets::Widgets() : lines(1.0f) {

	x_mov = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::x_mov), Vec3{0.0f, 0.0f, -90.0f},
	                    Color::red, Util::arrow_mesh(0.03f, 0.075f, 1.0f).to_gl()};
	y_mov = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::y_mov), Vec3{0.0f, 0.0f, 0.0f},
	                    Color::green, Util::arrow_mesh(0.03f, 0.075f, 1.0f).to_gl()};
	z_mov = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::z_mov), Vec3{90.0f, 0.0f, 0.0f},
	                    Color::blue, Util::arrow_mesh(0.03f, 0.075f, 1.0f).to_gl()};

	xy_mov = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::xy_mov), Vec3{-90.0f, 0.0f, 0.0f},
	                     Color::blue, Util::square_mesh(0.1f).to_gl()};
	yz_mov = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::yz_mov), Vec3{0.0f, 0.0f, -90.0f},
	                     Color::red, Util::square_mesh(0.1f).to_gl()};
	xz_mov = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::xz_mov), Vec3{0.0f, 0.0f, 0.0f},
	                     Color::green, Util::square_mesh(0.1f).to_gl()};

	x_rot = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::x_rot), Vec3{0.0f, 0.0f, -90.0f},
	                    Color::red, Util::torus_mesh(0.975f, 1.0f).to_gl()};
	y_rot = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::y_rot), Vec3{0.0f, 0.0f, 0.0f},
	                    Color::green, Util::torus_mesh(0.975f, 1.0f).to_gl()};
	z_rot = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::z_rot), Vec3{90.0f, 0.0f, 0.0f},
	                    Color::blue, Util::torus_mesh(0.975f, 1.0f).to_gl()};

	x_scl = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::x_scl), Vec3{0.0f, 0.0f, -90.0f},
	                    Color::red, Util::scale_mesh().to_gl()};
	y_scl = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::y_scl), Vec3{0.0f, 0.0f, 0.0f},
	                    Color::green, Util::scale_mesh().to_gl()};
	z_scl = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::z_scl), Vec3{90.0f, 0.0f, 0.0f},
	                    Color::blue, Util::scale_mesh().to_gl()};
	xyz_scl = Widget_Mesh{static_cast<uint32_t>(Widget_IDs::xyz_scl), Vec3{0.0f, 0.0f, 0.0f},
	                      Color::yellow, Util::cube_mesh(0.15f).to_gl()};
}

void Widgets::change_rot(Mat4 xf, Vec3 pose, Vec3 x, Vec3 y, Vec3 z) {
	x_rot.rot = (xf * Mat4::angle_axis(pose.z, z) * Mat4::angle_axis(pose.y, y) * Mat4::rotate_to(x)).to_euler();
	y_rot.rot = (xf * Mat4::angle_axis(pose.z, z) * Mat4::rotate_to(y)).to_euler();
	z_rot.rot = (xf * Mat4::rotate_to(z)).to_euler();
	// x_rot.rot = (xf * Mat4::rotate_to(x)).to_euler();
	// y_rot.rot = (xf * Mat4::rotate_to(y)).to_euler();
	// z_rot.rot = (xf * Mat4::rotate_to(z)).to_euler();
}

void Widgets::generate_lines(Vec3 pos) {

	const uint8_t axis_u = static_cast<uint8_t>(axis);

	auto add_axis = [&](uint8_t to_add) {
		Vec3 start = pos;
		start[to_add] -= 10000.0f;
		Vec3 end = pos;
		end[to_add] += 10000.0f;
		Spectrum color = Color::axis((Axis)to_add);
		lines.add(start, end, color);
	};

	if (drag_plane) {
		add_axis((axis_u + 1) % 3);
		add_axis((axis_u + 2) % 3);
	} else if (univ_scl) {
		add_axis(0);
		add_axis(1);
		add_axis(2);
	} else {
		add_axis(axis_u);
	}
}

void Widgets::render(const Mat4& view, Vec3 pos, float scl) {

	Renderer& r = Renderer::get();
	r.reset_depth();

	Vec3 scale(scl);
	r.lines(lines, view, Mat4::I, 0.5f);

	if (dragging && (active == Widget_Type::move || active == Widget_Type::scale)) return;

	auto render = [&](Widget_Mesh& w, Vec3 pos) {
		Renderer::MeshOpt opt;
		opt.id = w.id;
		opt.modelview = view * Mat4::translate(pos) * Mat4::euler(w.rot) * Mat4::scale(scale);
		opt.solid_color = true;
		opt.color = w.color;
		r.mesh(w.mesh, opt);
	};

	if (active == Widget_Type::move) {

		render(x_mov, pos + Vec3(0.15f * scl, 0.0f, 0.0f));
		render(y_mov, pos + Vec3(0.0f, 0.15f * scl, 0.0f));
		render(z_mov, pos + Vec3(0.0f, 0.0f, 0.15f * scl));
		render(xy_mov, pos + Vec3(0.45f * scl, 0.45f * scl, 0.0f));
		render(yz_mov, pos + Vec3(0.0f, 0.45f * scl, 0.45f * scl));
		render(xz_mov, pos + Vec3(0.45f * scl, 0.0f, 0.45f * scl));

	} else if (active == Widget_Type::rotate) {

		if (!dragging || axis == Axis::X) {
			render(x_rot, pos);
		}
		if (!dragging || axis == Axis::Y) {
			render(y_rot, pos);
		}
		if (!dragging || axis == Axis::Z) {
			render(z_rot, pos);
		}

	} else if (active == Widget_Type::scale) {

		render(x_scl, pos + Vec3(0.15f * scl, 0.0f, 0.0f));
		render(y_scl, pos + Vec3(0.0f, 0.15f * scl, 0.0f));
		render(z_scl, pos + Vec3(0.0f, 0.0f, 0.15f * scl));
		render(xyz_scl, pos);
	}
}

Transform Widgets::apply_action(const Transform& pose) {

	Transform result = pose;
	Vec3 vaxis;

	const uint8_t axis_u = static_cast<uint8_t>(axis);
	vaxis[axis_u] = 1.0f;

	switch (active) {
	case Widget_Type::move: {
		result.translation = pose.translation + drag_end - drag_start;
	} break;
	case Widget_Type::rotate: {
		Quat rot = Quat::axis_angle(vaxis, drag_end[axis_u]);
		result.rotation = rot * pose.rotation;
	} break;
	case Widget_Type::scale: {
		if (univ_scl) {
			result.scale = drag_end * pose.scale;
		} else {
			result.scale = Vec3{1.0f};
			result.scale[axis_u] = drag_end[axis_u];
			Mat4 rot = pose.rotation.to_mat();
			Mat4 trans =
				Mat4::transpose(rot) * Mat4::scale(result.scale) * rot * Mat4::scale(pose.scale);
			result.scale = Vec3(trans[0][0], trans[1][1], trans[2][2]);
		}
	} break;
	case Widget_Type::extrude:
	case Widget_Type::inset:
	case Widget_Type::bevel: {
		Vec2 off = bevel_start - bevel_end;
		result.translation = 2.0f * Vec3(off.x, -off.y, 0.0f);
	} break;
	default: assert(false);
	}

	return result;
}

bool Widgets::to_axis(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3& hit) {

	const uint8_t axis_u = static_cast<uint8_t>(axis);

	Vec3 axis1;
	axis1[axis_u] = 1.0f;
	Vec3 axis2;
	axis2[(axis_u + 1) % 3] = 1.0f;
	Vec3 axis3;
	axis3[(axis_u + 2) % 3] = 1.0f;

	Line select(cam_pos, dir);
	Line target(obj_pos, axis1);
	Plane l(obj_pos, axis2);
	Plane r(obj_pos, axis3);

	Vec3 hit1, hit2;
	bool hl = l.hit(select, hit1);
	bool hr = r.hit(select, hit2);
	if (!hl && !hr)
		return false;
	else if (!hl)
		hit = hit2;
	else if (!hr)
		hit = hit1;
	else
		hit = (hit1 - cam_pos).norm() > (hit2 - cam_pos).norm() ? hit2 : hit1;

	hit = target.closest(hit);
	return hit.valid();
}

bool Widgets::to_axis3(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3& hit) {

	Vec3 axis1{1.0f, 0.0f, 0.0f};
	Vec3 axis2{0.0f, 1.0f, 0.0f};
	Vec3 axis3{0.0f, 0.0f, 1.0f};

	Line select(cam_pos, dir);
	Line target(obj_pos, axis1);

	Plane k(obj_pos, axis1);
	Plane l(obj_pos, axis2);
	Plane r(obj_pos, axis3);

	Vec3 hit1, hit2, hit3;
	bool hk = k.hit(select, hit1);
	bool hl = l.hit(select, hit2);
	bool hr = r.hit(select, hit3);

	if (!hl && !hr && !hk) return false;

	Vec3 close{FLT_MAX};
	if (hk && (hit1 - cam_pos).norm() < (close - cam_pos).norm()) close = hit1;
	if (hl && (hit2 - cam_pos).norm() < (close - cam_pos).norm()) close = hit2;
	if (hr && (hit3 - cam_pos).norm() < (close - cam_pos).norm()) close = hit3;

	hit = close;
	return hit.valid();
}

bool Widgets::to_plane(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3 norm, Vec3& hit) {

	Line look(cam_pos, dir);
	Plane p(obj_pos, norm);
	return p.hit(look, hit);
}

bool Widgets::is_dragging() {
	return dragging;
}

bool Widgets::want_drag() {
	return start_dragging;
}

void Widgets::start_drag(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir) {

	start_dragging = false;
	dragging = true;

	Vec3 hit;
	Vec3 norm;

	const uint8_t axis_u = static_cast<uint8_t>(axis);
	norm[axis_u] = 1.0f;

	if (active == Widget_Type::rotate) {

		if (to_plane(pos, cam, dir, norm, hit)) {
			drag_start = (hit - pos).unit();
			drag_end = Vec3{0.0f};
		}

	} else {

		bool good;

		if (drag_plane)
			good = to_plane(pos, cam, dir, norm, hit);
		else if (univ_scl)
			good = to_axis3(pos, cam, dir, hit);
		else
			good = to_axis(pos, cam, dir, hit);

		if (!good) return;

		if (active == Widget_Type::bevel || active == Widget_Type::inset ||
		    active == Widget_Type::extrude) {
			bevel_start = bevel_end = spos;
		} else if (active == Widget_Type::move) {
			drag_start = drag_end = hit;
		} else {
			drag_start = hit;
			drag_end = Vec3{1.0f};
		}

		if (active != Widget_Type::bevel && active != Widget_Type::extrude &&
		    active != Widget_Type::inset)
			generate_lines(pos);
	}
}

void Widgets::end_drag() {
	lines.clear();
	drag_start = drag_end = {};
	bevel_start = bevel_end = {};
	dragging = false;
	drag_plane = false;
	univ_scl = false;
}

void Widgets::drag_to(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir, bool scale_invert, float snap) {

	Vec3 hit;
	Vec3 norm;

	const uint8_t axis_u = static_cast<uint8_t>(axis);
	norm[axis_u] = 1.0f;

	if (active == Widget_Type::bevel || active == Widget_Type::extrude ||
	    active == Widget_Type::inset) {

		bevel_end = spos;

	} else if (active == Widget_Type::rotate) {

		if (!to_plane(pos, cam, dir, norm, hit)) return;

		Vec3 ang = (hit - pos).unit();
		float sgn = sign(cross(drag_start, ang)[axis_u]);
		drag_end = Vec3{};
		drag_end[axis_u] = sgn * Degrees(std::acos(dot(drag_start, ang)));
		if (snap != 0.0f) {
			drag_end[axis_u] = std::round(drag_end[axis_u] / snap) * snap;
		}

	} else {

		bool good;

		if (drag_plane)
			good = to_plane(pos, cam, dir, norm, hit);
		else if (univ_scl)
			good = to_axis3(pos, cam, dir, hit);
		else
			good = to_axis(pos, cam, dir, hit);

		if (!good) return;

		if (active == Widget_Type::move) {
			if (snap != 0.0f) {
				hit.x = std::round((hit.x - drag_start.x) / snap) * snap + drag_start.x;
				hit.y = std::round((hit.y - drag_start.y) / snap) * snap + drag_start.y;
				hit.z = std::round((hit.z - drag_start.z) / snap) * snap + drag_start.z;
			}
			drag_end = hit;
		} else if (univ_scl && active == Widget_Type::scale) {
			float f = (hit - pos).norm() / (drag_start - pos).norm();
			drag_end = Vec3(std::sqrt(f));
		} else if (active == Widget_Type::scale) {
			drag_end = Vec3{1.0f};
			drag_end[axis_u] = (hit - pos).norm() / (drag_start - pos).norm();
		} else
			assert(false);
	}

	if (scale_invert && active == Widget_Type::scale && !univ_scl) {
		drag_end[axis_u] *= sign(dot(hit - pos, drag_start - pos));
	}
}

void Widgets::action_button(Widget_Type act, const std::string& name, bool wrap) {
	using namespace ImGui;
	bool is_active = act == active;
	if (is_active) PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
	bool clicked = wrap ? WrapButton(name) : Button(name.c_str());
	if (is_active) PopStyleColor();
	if (clicked) active = act;
};

bool Widgets::select(uint32_t id) {

	start_dragging = true;
	drag_plane = false;
	univ_scl = false;

	switch (id) {
	case static_cast<uint32_t>(Widget_IDs::x_mov): {
		active = Widget_Type::move;
		axis = Axis::X;
	} break;
	case static_cast<uint32_t>(Widget_IDs::y_mov): {
		active = Widget_Type::move;
		axis = Axis::Y;
	} break;
	case static_cast<uint32_t>(Widget_IDs::z_mov): {
		active = Widget_Type::move;
		axis = Axis::Z;
	} break;
	case static_cast<uint32_t>(Widget_IDs::xy_mov): {
		active = Widget_Type::move;
		axis = Axis::Z;
		drag_plane = true;
	} break;
	case static_cast<uint32_t>(Widget_IDs::yz_mov): {
		active = Widget_Type::move;
		axis = Axis::X;
		drag_plane = true;
	} break;
	case static_cast<uint32_t>(Widget_IDs::xz_mov): {
		active = Widget_Type::move;
		axis = Axis::Y;
		drag_plane = true;
	} break;
	case static_cast<uint32_t>(Widget_IDs::x_rot): {
		active = Widget_Type::rotate;
		axis = Axis::X;
	} break;
	case static_cast<uint32_t>(Widget_IDs::y_rot): {
		active = Widget_Type::rotate;
		axis = Axis::Y;
	} break;
	case static_cast<uint32_t>(Widget_IDs::z_rot): {
		active = Widget_Type::rotate;
		axis = Axis::Z;
	} break;
	case static_cast<uint32_t>(Widget_IDs::x_scl): {
		active = Widget_Type::scale;
		axis = Axis::X;
	} break;
	case static_cast<uint32_t>(Widget_IDs::y_scl): {
		active = Widget_Type::scale;
		axis = Axis::Y;
	} break;
	case static_cast<uint32_t>(Widget_IDs::z_scl): {
		active = Widget_Type::scale;
		axis = Axis::Z;
	} break;
	case static_cast<uint32_t>(Widget_IDs::xyz_scl): {
		active = Widget_Type::scale;
		axis = Axis::X;
		univ_scl = true;
	} break;
	default: {
		start_dragging = false;
	} break;
	}

	return start_dragging;
}

Widget_Type Widget_Transform::ui(Widget_Type active, Undo& undo, std::weak_ptr<Transform> apply_to) {

	using namespace ImGui;
	if (apply_to.expired()) return active;

	Transform& _new = *apply_to.lock();
	Vec3 euler = _new.rotation.to_euler();

	auto sliders = [&](Widget_Type act, const std::string& label, Vec3& data, float sens) {
		if (DragFloat3(label.c_str(), data.data, sens)) active = act;
		if (IsItemActivated()) {
			cache = _new;
			euler = cache.rotation.to_euler();
		}
		if (IsItemDeactivatedAfterEdit() && cache != _new) {
			_new.rotation = Quat::euler(euler);
			undo.update<Transform>(apply_to, cache);
		}
	};

	auto action_button = [&](Widget_Type act, const std::string& name, bool wrap = true) {
		bool is_active = act == active;
		if (is_active) PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
		bool clicked = wrap ? WrapButton(name) : Button(name.c_str());
		if (is_active) PopStyleColor();
		if (clicked) active = act;
	};

	sliders(Widget_Type::move, "Translation", _new.translation, 0.1f);
	sliders(Widget_Type::rotate, "Rotation", euler, 1.0f);
	sliders(Widget_Type::scale, "Scale", _new.scale, 0.03f);

	action_button(Widget_Type::move, "Move [m]", false);
	action_button(Widget_Type::rotate, "Rotate [r]");
	action_button(Widget_Type::scale, "Scale [s]");

	_new.rotation = Quat::euler(euler);

	return active;
}

void Widget_Transform::update(Undo& undo, std::weak_ptr<Transform> t) {
	Transform& _new = *t.lock();
	if (cache != _new) undo.update<Transform>(t, cache);
}

void Widget_Camera::ui(Undo& undo, const std::string& name, std::weak_ptr<Camera> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	Camera& camera = *apply_to.lock();

	bool update = false;

	// Don't ask
	bool any_activated = ar_activated || fov_activated || near_activated || 
			aperture_activated || focal_dist_activated || ray_depth_activated;
	auto slider = [&](bool changed, bool& activated) {
		if (IsItemActivated()) {
			activated = true;
			any_activated = true;
		}
		if (!IsItemActive()) {
			if (activated) {
				update = cache != camera;
			} else if (!any_activated) {
				cache = camera;
			}
			activated = false;
		}
		if (changed) undo.invalidate(name);
	};

	auto check = [&]() {
		if (IsItemActivated()) cache = camera;
		if (IsItemDeactivated() && cache != camera) update = true;
	};

	const char* label = nullptr;
	SamplePattern const *sample_pattern = SamplePattern::from_id(camera.film.sample_pattern);
	if (sample_pattern != nullptr) {
		label = sample_pattern->name.c_str();
	}
	if (BeginCombo("Sample Pattern", label)) {
		for (SamplePattern const &sp : SamplePattern::all_patterns()) {
			if(Selectable(sp.name.c_str())) {
				label = sp.name.c_str();
				cache = camera;
				camera.film.sample_pattern = sp.id;
				update = cache != camera;
				break;
			}
		}
		EndCombo();
	}

	slider(SliderFloat("Aspect Ratio", &camera.aspect_ratio, 0.1f, 10.0f, "%.2f",
	                   ImGuiSliderFlags_Logarithmic),
	       ar_activated);
	slider(SliderFloat("Vertical FOV", &camera.vertical_fov, 10.0f, 160.0f, "%.2f"), fov_activated);
	slider(SliderFloat("Near Plane", &camera.near_plane, 0.001f, 1.0f, "%.3f",
	                   ImGuiSliderFlags_Logarithmic),
	       near_activated);

	const char* label_aperture = nullptr;
	ApertureShape const *aperture_shape = ApertureShape::from_id(camera.aperture_shape);
	if (aperture_shape != nullptr) {
		label_aperture = aperture_shape->name.c_str();
	}
	if (BeginCombo("Aperture Shape", label_aperture)) {
		for (ApertureShape const &as : ApertureShape::all_shapes()) {
			if(Selectable(as.name.c_str())) {
				label = as.name.c_str();
				cache = camera;
				camera.aperture_shape = as.id;
				update = cache != camera;
				break;
			}
		}
		EndCombo();
	}

	slider(SliderFloat("Aperture Size", &camera.aperture_size, 0.f, 0.2f, "%.2f"), aperture_activated);
	slider(SliderFloat("Focal Distance", &camera.focal_dist, 0.2f, 10.f, "%.2f"), focal_dist_activated);

	slider(SliderUInt32("Ray Depth", &camera.film.max_ray_depth, 1, 20), ray_depth_activated);

	InputUInt32("Film Width", &camera.film.width);
	check();
	InputUInt32("Film Height", &camera.film.height);
	check();
	InputUInt32("Film Samples", &camera.film.samples);
	check();

	if (Button("Compute Width")) {
		cache = camera;
		camera.film.width = static_cast<uint32_t>(camera.film.height * camera.aspect_ratio);
		update = cache != camera;
	}
	if (WrapButton("Compute Height")) {
		cache = camera;
		camera.film.height = static_cast<uint32_t>(camera.film.width / camera.aspect_ratio);
		update = cache != camera;
	}
	if (WrapButton("Compute Aspect Ratio")) {
		cache = camera;
		camera.aspect_ratio = static_cast<float>(camera.film.width) / static_cast<float>(camera.film.height);
		update = cache != camera;
	}

	camera.aspect_ratio = std::clamp(camera.aspect_ratio, 0.1f, 10.0f);
	camera.vertical_fov = std::clamp(camera.vertical_fov, 10.0f, 160.0f);
	camera.near_plane = std::clamp(camera.near_plane, 0.001f, 1.0f);
	camera.aperture_size = std::clamp(camera.aperture_size, 0.0f, 1.0f);
	camera.focal_dist = std::clamp(camera.focal_dist, 0.01f, 100.0f);

	if (update) undo.update_cached<Camera>(name, apply_to, cache);
}

void Widget_Delta_Light::ui(const std::string& name, Manager& manager, Undo& undo,
                            std::weak_ptr<Delta_Light> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	auto light = apply_to.lock();

	bool update = false;

	bool any_activated =
		color_activated || intensity_activated || inner_activated || outer_activated;
	auto activate = [&](bool changed, bool& activated) {
		if (changed) manager.invalidate_gpu(name);
		if (IsItemActivated()) {
			activated = true;
			any_activated = true;
		}
		if (!IsItemActive()) {
			if (activated) {
				update = old != *light;
			} else if (!any_activated) {
				old = std::move(*light);
			}
			activated = false;
		}
	};

	bool is_point = light->is<Delta_Lights::Point>();
	bool is_directional = light->is<Delta_Lights::Directional>();
	bool is_spot = light->is<Delta_Lights::Spot>();

	const char* preview = nullptr;
	if (is_point) preview = "Point";
	if (is_directional) preview = "Directional";
	if (is_spot) preview = "Spot";

	if (BeginCombo("Type##delta_light", preview)) {
		if (Selectable("Point", is_point)) {
			if (!is_point) {
				old = std::move(*light);
				light->light = Delta_Lights::Point{};
				undo.update_cached<Delta_Light>(name, apply_to, std::move(old));
			}
		}
		if (Selectable("Directional", is_directional)) {
			if (!is_directional) {
				old = std::move(*light);
				light->light = Delta_Lights::Directional{};
				undo.update_cached<Delta_Light>(name, apply_to, std::move(old));
			}
		}
		if (Selectable("Spot", is_spot)) {
			if (!is_spot) {
				old = std::move(*light);
				light->light = Delta_Lights::Spot{};
				undo.update_cached<Delta_Light>(name, apply_to, std::move(old));
			}
		}
		EndCombo();
	}

	if (light->is<Delta_Lights::Point>()) {
		auto& point = std::get<Delta_Lights::Point>(light->light);
		activate(ColorEdit3("Color", point.color.data), color_activated);
		activate(DragFloat("Intensity", &point.intensity, 1.0f, 0.0f,
		                   std::numeric_limits<float>::max(), "%.2f"),
		         intensity_activated);
	}
	if (light->is<Delta_Lights::Directional>()) {
		auto& directional = std::get<Delta_Lights::Directional>(light->light);
		activate(ColorEdit3("Color", directional.color.data), color_activated);
		activate(DragFloat("Intensity", &directional.intensity, 1.0f, 0.0f,
		                   std::numeric_limits<float>::max(), "%.2f"),
		         intensity_activated);
	}
	if (light->is<Delta_Lights::Spot>()) {
		auto& spot = std::get<Delta_Lights::Spot>(light->light);
		activate(ColorEdit3("Color", spot.color.data), color_activated);
		activate(DragFloat("Intensity", &spot.intensity, 1.0f, 0.0f,
		                   std::numeric_limits<float>::max(), "%.2f"),
		         intensity_activated);
		activate(DragFloat("Inner Falloff", &spot.inner_angle, 1.0f, 0.0f, 360.0f),
		         inner_activated);
		activate(DragFloat("Outer Falloff", &spot.outer_angle, 1.0f, 0.0f, 360.0f),
		         outer_activated);

		spot.inner_angle = std::clamp(spot.inner_angle, 0.0f, spot.outer_angle);
	}

	if (update) {
		undo.update_cached<Delta_Light>(name, apply_to, std::move(old));
	}
}

void Widget_Environment_Light::ui(Manager& manager, Undo& undo,
                                  std::weak_ptr<Environment_Light> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	auto env_light = apply_to.lock();

	bool update = false;

	bool is_hemisphere = env_light->is<Environment_Lights::Hemisphere>();
	bool is_sphere = env_light->is<Environment_Lights::Sphere>();

	const char* preview = nullptr;
	if (is_hemisphere) preview = "Hemisphere";
	if (is_sphere) preview = "Sphere";

	if (BeginCombo("Type##envlight", preview)) {
		if (Selectable("Hemisphere", is_hemisphere)) {
			if (!is_hemisphere) {
				old = std::move(*env_light);
				env_light->light = Environment_Lights::Hemisphere{};
				undo.update<Environment_Light>(apply_to, std::move(old));
			}
		}
		if (Selectable("Sphere", is_sphere)) {
			if (!is_sphere) {
				old = std::move(*env_light);
				env_light->light = Environment_Lights::Sphere{};
				undo.update<Environment_Light>(apply_to, std::move(old));
			}
		}
		EndCombo();
	}

	if (env_light->is<Environment_Lights::Hemisphere>()) {
		auto& hemisphere = std::get<Environment_Lights::Hemisphere>(env_light->light);
		manager.choose_instance("Radiance", env_light, hemisphere.radiance);
		manager.edit_texture(hemisphere.radiance);
	}
	if (env_light->is<Environment_Lights::Sphere>()) {
		auto& sphere = std::get<Environment_Lights::Sphere>(env_light->light);
		manager.choose_instance("Radiance", env_light, sphere.radiance);
		manager.edit_texture(sphere.radiance);
	}

	if (update) {
		undo.update<Environment_Light>(apply_to, std::move(old));
	}
}

void Widget_Material::ui(Manager& manager, Undo& undo, std::weak_ptr<Material> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	auto material = apply_to.lock();

	bool update = false;

	auto activate = [&](bool changed, bool& activated) {
		if (IsItemActivated()) {
			activated = true;
			old = *material;
		}
		if (!IsItemActive()) {
			if (activated) {
				update = old != *material;
			}
			activated = false;
		}
	};

	bool is_lambertian = material->is<Materials::Lambertian>();
	bool is_mirror = material->is<Materials::Mirror>();
	bool is_refract = material->is<Materials::Refract>();
	bool is_glass = material->is<Materials::Glass>();
	bool is_emissive = material->is<Materials::Emissive>();

	const char* preview = nullptr;
	if (is_lambertian) preview = "Lambertian";
	if (is_mirror) preview = "Mirror";
	if (is_refract) preview = "Refract";
	if (is_glass) preview = "Glass";
	if (is_emissive) preview = "Emissive";

	if (BeginCombo("Type##material", preview)) {
		if (Selectable("Lambertian", is_lambertian)) {
			if (!is_lambertian) {
				old = std::move(*material);
				material->material = Materials::Lambertian{};
				undo.update<Material>(apply_to, std::move(old));
			}
		}
		if (Selectable("Mirror", is_mirror)) {
			if (!is_mirror) {
				old = std::move(*material);
				material->material = Materials::Mirror{};
				undo.update<Material>(apply_to, std::move(old));
			}
		}
		if (Selectable("Refract", is_refract)) {
			if (!is_refract) {
				old = std::move(*material);
				material->material = Materials::Refract{};
				undo.update<Material>(apply_to, std::move(old));
			}
		}
		if (Selectable("Glass", is_glass)) {
			if (!is_glass) {
				old = std::move(*material);
				material->material = Materials::Glass{};
				undo.update<Material>(apply_to, std::move(old));
			}
		}
		if (Selectable("Emissive", is_emissive)) {
			if (!is_emissive) {
				old = std::move(*material);
				material->material = Materials::Emissive{};
				undo.update<Material>(apply_to, std::move(old));
			}
		}
		EndCombo();
	}

	if (material->is<Materials::Lambertian>()) {
		auto& lambertian = std::get<Materials::Lambertian>(material->material);
		manager.choose_instance("Albedo", material, lambertian.albedo);
		manager.edit_texture(lambertian.albedo);
	}
	if (material->is<Materials::Mirror>()) {
		auto& mirror = std::get<Materials::Mirror>(material->material);
		manager.choose_instance("Reflectance", material, mirror.reflectance);
		manager.edit_texture(mirror.reflectance);
	}
	if (material->is<Materials::Refract>()) {
		auto& refract = std::get<Materials::Refract>(material->material);
		manager.choose_instance("Transmittance", material, refract.transmittance);
		manager.edit_texture(refract.transmittance);
		activate(
			DragFloat("IOR", &refract.ior, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f"),
			ior_activated);
	}
	if (material->is<Materials::Glass>()) {
		auto& glass = std::get<Materials::Glass>(material->material);
		manager.choose_instance("Reflectance", material, glass.reflectance);
		manager.edit_texture(glass.reflectance);
		manager.choose_instance("Transmittance", material, glass.transmittance);
		manager.edit_texture(glass.transmittance);
		activate(
			DragFloat("IOR", &glass.ior, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f"),
			ior_activated);
	}
	if (material->is<Materials::Emissive>()) {
		auto& emissive = std::get<Materials::Emissive>(material->material);
		manager.choose_instance("Emission", material, emissive.emissive);
		manager.edit_texture(emissive.emissive);
	}

	if (update) {
		undo.update<Material>(material, std::move(old));
	}
}

void Widget_Shape::ui(const std::string& name, Undo& undo, std::weak_ptr<Shape> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	auto shape = apply_to.lock();

	bool update = false;
	auto check = [&]() {
		if (IsItemActivated()) old = std::move(*shape);
		if (IsItemDeactivated() && old != *shape) update = true;
	};

	bool is_sphere = shape->is<Shapes::Sphere>();

	if (BeginCombo("Type##shape", "Sphere")) {
		if (Selectable("Sphere", is_sphere)) {
		}
		EndCombo();
	}

	if (shape->is<Shapes::Sphere>()) {
		auto& sphere = std::get<Shapes::Sphere>(shape->shape);
		DragFloat("Radius", &sphere.radius, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
		check();
	}

	if (update) {
		undo.update_cached<Shape>(name, apply_to, old);
	}
}

void Widget_Particles::ui(Undo& undo, std::weak_ptr<Particles> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	auto particles = apply_to.lock();

	bool update = false;

	auto temp_particles = std::move(particles->particles);

	bool any_activated = gravity_activated || radius_activated || initial_velocity_activated ||
	                     spread_angle_activated || lifetime_activated || pps_activated ||
	                     step_size_activated;
	auto activate = [&](bool changed, bool& activated) {
		if (IsItemActivated()) {
			activated = true;
			any_activated = true;
		}
		if (!IsItemActive()) {
			if (activated) {
				update = old != *particles;
			} else if (!any_activated) {
				old = std::move(*particles);
			}
			activated = false;
		}
	};

	activate(DragFloat3("Gravity", &particles->gravity.x, 0.1f), gravity_activated);
	activate(DragFloat("Radius", &particles->radius, 0.01f, 1e-4f, std::numeric_limits<float>::max(), "%.2f"), radius_activated);
	activate(DragFloat("Initial Velocity", &particles->initial_velocity, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f"), initial_velocity_activated);
	activate(SliderFloat("Spread Angle", &particles->spread_angle, 0.0f, 180.0f, "%.2f"), spread_angle_activated);
	activate(DragFloat("Lifetime", &particles->lifetime, 0.01f, 0.0f, std::numeric_limits<float>::max(), "%.2f"), lifetime_activated);
	activate(DragFloat("Particles/Sec", &particles->rate, 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f"), pps_activated);
	activate(DragFloat("Timestep", &particles->step_size, 0.001f, 0.0f, std::numeric_limits<float>::max(), "%.4f"), step_size_activated);

	if (Button("Clear##particles")) {
		temp_particles.clear();
	}

	if (update) {
		old.particles = std::move(temp_particles);
		undo.update<Particles>(apply_to, std::move(old));
	}

	particles->particles = std::move(temp_particles);
}

void Widget_Texture::ui(const std::string& name, Manager& manager, Undo& undo,
                        std::weak_ptr<Texture> apply_to) {

	using namespace ImGui;

	if (apply_to.expired()) return;
	std::shared_ptr<Texture> texture = apply_to.lock();

	bool update = false;

	bool any_activated = color_activated || scale_activated;
	auto activate = [&](bool changed, bool& activated) {
		if (IsItemActivated()) {
			activated = true;
			any_activated = true;
		}
		if (!IsItemActive()) {
			if (activated) {
				update = old != *texture;
			} else if (!any_activated) {
				old = std::move(*texture);
			}
			activated = false;
		}
	};

	bool is_constant = texture->is<Textures::Constant>();
	bool is_image = texture->is<Textures::Image>();

	if (BeginCombo("Type##texture", is_constant ? "Constant" : "Image")) {
		if (Selectable("Constant", is_constant)) {
			if (!is_constant) {
				old = std::move(*texture);
				texture->texture = Textures::Constant{};
				undo.update_cached<Texture>(name, apply_to, std::move(old));
			}
		}
		if (Selectable("Image", is_image)) {
			if (!is_image) {
				old = std::move(*texture);
				texture->texture = Textures::Image{};
				undo.update_cached<Texture>(name, apply_to, std::move(old));
			}
		}
		EndCombo();
	}

	if (Textures::Constant *const_ptr = std::get_if<Textures::Constant>(&texture->texture)) {
		auto &const_ = *const_ptr;


		activate(ColorEdit3("Color", const_.color.data), color_activated);
		activate(DragFloat("Scale", &const_.scale, 0.1f, 0.0f, std::numeric_limits<float>::max(),
		                   "%.2f"),
		         scale_activated);

	} else if (Textures::Image *img_ptr = std::get_if<Textures::Image>(&texture->texture)) {
		auto &img = *img_ptr;

		std::string selected;
		if        (img.sampler == Textures::Image::Sampler::nearest) {
			selected = "Nearest";
		} else if (img.sampler == Textures::Image::Sampler::bilinear) {
			selected = "Bilinear";
		} else if (img.sampler == Textures::Image::Sampler::trilinear) {
			selected = "Trilinear";
		} else {
			selected = "Nearest";
			warn("Unknown image sampler type.");
		}

		if (BeginCombo("Type##sampler", selected.c_str())) {
			if (Selectable("Nearest", selected == "Nearest")) {
				try {
					img.sampler = Textures::Image::Sampler::nearest;
					img.update_mipmap();
					update = old != *texture;
				} catch (std::exception const& e) {
					*texture = std::move(old);
					manager.set_error(e.what());
				}
			}
			if (Selectable("Bilinear", selected == "Bilinear")) {
				try {
					img.sampler = Textures::Image::Sampler::bilinear;
					img.update_mipmap();
					update = old != *texture;
				} catch (std::exception const& e) {
					*texture = std::move(old);
					manager.set_error(e.what());
				}
			}
			if (Selectable("Trilinear", selected == "Trilinear")) {
				try {
					img.sampler = Textures::Image::Sampler::trilinear;
					img.update_mipmap();
					update = old != *texture;
				} catch (std::exception const& e) {
					*texture = std::move(old);
					manager.set_error(e.what());
				}
			}
			EndCombo();
		}

		//maximum draw size:
		float x = GetContentRegionAvail().x;
		float y = 0.5f * x;
		//now shrink to match aspect ratio:
		float aspect = img.image.h / float(img.image.w);
		if (x * aspect < y) {
			y = x * aspect;
		} else {
			x = y / aspect;
		}
		manager.render_image(name, Vec2{x, y});
		if (Button("Change")) {
			if (auto path = manager.choose_image()) {
				old = std::move(*texture);
				try {
					img.image = HDR_Image::load(path.value());
					img.update_mipmap();
					update = old != *texture;
				} catch (std::exception const& e) {
					*texture = std::move(old);
					manager.set_error(e.what());
				}
			}
		}
	}

	if (update) {
		undo.update_cached<Texture>(name, apply_to, std::move(old));
	}
}

void Halfedge_Mesh_Controls::ui(std::function< void(std::function< void(Halfedge_Mesh &) > const &) > const &apply) {
	using namespace ImGui;

	if (Button("Flip Orientation")) {
		apply([]( Halfedge_Mesh &mesh ){
			mesh.flip_orientation();
		});
	}
	{ //normal smoothing:
		SliderFloat("##Smooth Angle", &corner_normals_angle, 0.0f, 180.0f, "%.1f deg");
		if (WrapButton("Smooth Normals")) {
			apply([this]( Halfedge_Mesh &mesh ){
				mesh.set_corner_normals(corner_normals_angle);
			});
		}
	}
	if (Button("UVs XY")) {
		apply([]( Halfedge_Mesh &mesh ){
			mesh.set_corner_uvs_project(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
		});
	}
	SameLine();
	if (WrapButton("XZ")) {
		apply([]( Halfedge_Mesh &mesh ){
			mesh.set_corner_uvs_project(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
		});
	}
	SameLine();
	if (Button("YZ")) {
		apply([]( Halfedge_Mesh &mesh ){
			mesh.set_corner_uvs_project(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
		});
	}
	SameLine();
	if (Button("Per-Face")) {
		apply([]( Halfedge_Mesh &mesh ){
			mesh.set_corner_uvs_per_face();
		});
	}
	if (auto new_shape = create_shape()) {
		apply([&new_shape]( Halfedge_Mesh &mesh ){
			mesh = std::move(new_shape.value());
		});
	}
}

std::optional<Halfedge_Mesh> Halfedge_Mesh_Controls::create_shape() {
	using namespace ImGui;

	const char* label = nullptr;
	switch (type) {
	case Util::Shape::cube: label = "Cube"; break;
	case Util::Shape::square: label = "Square"; break;
	case Util::Shape::pentagon: label = "Pentagon"; break;
	case Util::Shape::cylinder: label = "Cylinder"; break;
	case Util::Shape::torus: label = "Torus"; break;
	case Util::Shape::cone: label = "Cone"; break;
	case Util::Shape::closed_sphere: label = "Closed Sphere"; break;
	case Util::Shape::texture_sphere: label = "Texture Sphere"; break;
	default: die("Unknown shape type");
	}
	if (BeginCombo("##create-combo", label)) {
		if (Selectable("Cube")) type = Util::Shape::cube;
		if (Selectable("Square")) type = Util::Shape::square;
		if (Selectable("Pentagon")) type = Util::Shape::pentagon;
		if (Selectable("Cylinder")) type = Util::Shape::cylinder;
		if (Selectable("Torus")) type = Util::Shape::torus;
		if (Selectable("Cone")) type = Util::Shape::cone;
		if (Selectable("Closed Sphere")) type = Util::Shape::closed_sphere;
		if (Selectable("Texture Sphere")) type = Util::Shape::texture_sphere;
		EndCombo();
	}

	if (WrapButton("Replace")) {
		switch (type) {
		case Util::Shape::cube:
			return Halfedge_Mesh::cube(cube_radius);
		case Util::Shape::square:
			return Halfedge_Mesh::from_indexed_mesh(Util::square_mesh(square_radius));
		case Util::Shape::pentagon:
			return Halfedge_Mesh::from_indexed_mesh(Util::pentagon_mesh(pentagon_radius));
		case Util::Shape::cylinder:
			return Halfedge_Mesh::from_indexed_mesh(Util::cyl_mesh(cylinder_radius, cylinder_height, cylinder_sides));
		case Util::Shape::torus:
			return Halfedge_Mesh::from_indexed_mesh(Util::torus_mesh(torus_inner_radius, torus_outer_radius, torus_sides, torus_rings));
		case Util::Shape::cone:
			return Halfedge_Mesh::from_indexed_mesh(Util::cone_mesh(cone_bottom_radius, cone_top_radius, cone_height, cone_sides));
		case Util::Shape::closed_sphere:
			return Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(sphere_radius, sphere_subdivisions));
		case Util::Shape::texture_sphere:
			return Halfedge_Mesh::from_indexed_mesh(Util::texture_sphere_mesh(sphere_radius, sphere_subdivisions));
		default:
			die("Unknown shape type");
		}
	}

	switch (type) {
	case Util::Shape::cube: {
		SliderFloat("Radius##cube", &cube_radius, 0.01f, 10.0f, "%.2f");
	} break;
	case Util::Shape::square: {
		SliderFloat("Radius##square", &square_radius, 0.01f, 10.0f, "%.2f");
	} break;
	case Util::Shape::cylinder: {
		SliderFloat("Radius##cylinder", &cylinder_radius, 0.01f, 10.0f, "%.2f");
		SliderFloat("Height##cylinder", &cylinder_height, 0.01f, 10.0f, "%.2f");
		SliderUInt32("Sides##cylinder", &cylinder_sides, 3, 100);
	} break;
	case Util::Shape::torus: {
		SliderFloat("Inner Radius##torus", &torus_inner_radius, 0.01f, 10.0f, "%.2f");
		SliderFloat("Outer Radius##torus", &torus_outer_radius, 0.01f, 10.0f, "%.2f");
		SliderUInt32("Sides##torus", &torus_sides, 3, 100);
		SliderUInt32("Rings##torus", &torus_rings, 3, 100);
	} break;
	case Util::Shape::cone: {
		SliderFloat("Bottom Radius##cone", &cone_bottom_radius, 0.01f, 10.0f, "%.2f");
		SliderFloat("Top Radius##cone", &cone_top_radius, 0.01f, 10.0f, "%.2f");
		SliderFloat("Height##cone", &cone_height, 0.01f, 10.0f, "%.2f");
		SliderUInt32("Sides##cone", &cone_sides, 3, 100);
	} break;
	case Util::Shape::closed_sphere: {
		SliderFloat("Radius##sphere", &sphere_radius, 0.01f, 10.0f, "%.2f");
		SliderUInt32("Subdivisions##sphere", &sphere_subdivisions, 0, 10);
	} break;
	case Util::Shape::texture_sphere: {
		SliderFloat("Radius##sphere", &sphere_radius, 0.01f, 10.0f, "%.2f");
		SliderUInt32("Subdivisions##sphere", &sphere_subdivisions, 0, 10);
	} break;
	case Util::Shape::pentagon: {
		SliderFloat("Radius##pentagon", &pentagon_radius, 0.01f, 10.0f, "%.2f");
	} break;
	default: die("Unknown shape type");
	}
	return std::nullopt;
}

Mode Widget_Halfedge_Mesh::ui(Mode current, const std::string& name, Undo& undo, std::weak_ptr<Halfedge_Mesh> apply_to) {
	using namespace ImGui;

	auto mesh = apply_to.lock();
	if (!mesh) return current;

	if (Button("Edit")) current = Mode::model;
	SameLine();
	ui(name, undo, apply_to);

	return current;
}

void Widget_Halfedge_Mesh::ui(const std::string& name, Undo& undo, std::weak_ptr<Halfedge_Mesh> apply_to) {
	auto mesh = apply_to.lock();
	if (!mesh) return;

	controls.ui( [&]( std::function< void(Halfedge_Mesh &) > const &edit ){
		Halfedge_Mesh old = mesh->copy();
		edit(*mesh);
		undo.update_cached< Halfedge_Mesh >(name, mesh, std::move(old));
	});
}

Mode Widget_Skinned_Mesh::ui(Mode current, const std::string& name, Undo& undo, std::weak_ptr<Skinned_Mesh> apply_to) {
	using namespace ImGui;

	auto mesh = apply_to.lock();
	if (!mesh) return current;

	if (Button("Edit Mesh")) current = Mode::model;
	if (WrapButton("Edit Skeleton")) current = Mode::rig;
	ui(name, undo, apply_to);

	return current;
}

void Widget_Skinned_Mesh::ui(const std::string& name, Undo& undo, std::weak_ptr<Skinned_Mesh> apply_to) {
	using namespace ImGui;

	auto mesh = apply_to.lock();
	if (!mesh) return;

	controls.ui( [&]( std::function< void(Halfedge_Mesh &) > const &edit ){
		Skinned_Mesh old = mesh->copy();
		edit(mesh->mesh);
		undo.update_cached<Skinned_Mesh>(name, mesh, std::move(old));
	});
}

bool Widget_Render::in_progress() {
	return pathtracer.in_progress() || (rasterizer && rasterizer->in_progress()) || rendering_animation;
}

void Widget_Render::open(Scene &scene) {
	render_window = true;
	render_window_focus = true;
	//auto-select render camera:
	if (render_cam.expired()) {
		render_cam.reset();
		std::string best = "";
		for (auto& [name, cam] : scene.instances.cameras) {
			if (render_cam.expired() || name < best) {
				best = name;
				render_cam = cam;
			}
		}
	}
}

void Widget_Render::begin_window(Scene& scene, Undo& undo, Manager& manager, View_3D& gui_cam) {

	using namespace ImGui;

	if (render_window_focus) {
		SetNextWindowFocus();
		render_window_focus = false;
	}
	SetNextWindowSize({675.0f, 625.0f}, ImGuiCond_Once);
	Begin("Render Image", &render_window, ImGuiWindowFlags_NoCollapse);

	Combo("Method", &method, Method_Names);

	{
		auto render_cam_name = scene.name<Instance::Camera>(render_cam);

		if (BeginCombo("Camera Instance##render-combo",
		               render_cam_name ? render_cam_name.value().c_str() : "None")) {
			for (auto& [name, _] : scene.instances.cameras) {
				bool is_selected = name == render_cam_name;
				if (Selectable(name.c_str(), is_selected)) render_cam_name = name;
				if (is_selected) SetItemDefaultFocus();
			}
			if (Selectable("New...", false)) {
				render_cam_name = undo.create("Camera Instance", Instance::Camera{scene.get<Transform>(undo.create("Transform", Transform{})), 
																						scene.get<Camera>(undo.create("Camera", Camera{}))});
			}
			EndCombo();
		}

		render_cam = render_cam_name ? scene.get<Instance::Camera>(render_cam_name.value())
		                             : std::weak_ptr<Instance::Camera>{};

		if (auto r = render_cam.lock()) {
			manager.choose_instance("Transform", r, r->transform);
			manager.edit_camera(r, gui_cam);
		}
	}

	if (SliderFloat("Exposure", &exposure, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
		std::lock_guard<std::mutex> lock(report_mut);
		display = display_hdr.to_gl(exposure);
	}

	if (method == Method::path_trace) {
		Checkbox("Use BVH", &use_bvh);
	}
}

void Widget_Render::ui_animate(Scene& scene, Manager& manager, Undo& undo, View_3D& gui_cam,
                               uint32_t last_frame) {

	using namespace ImGui;

	if (!render_window) return;

	begin_window(scene, undo, manager, gui_cam);

	if (Button("Output Folder")) {
		char* path = nullptr;
		NFD_OpenDirectoryDialog(nullptr, nullptr, &path);
		if (path) {
			Platform::strcpy(output_path_buffer, path, sizeof(output_path_buffer));
			free(path);
		}
	}
	SameLine();
	InputText("##path", output_path_buffer, sizeof(output_path_buffer));

	Separator();
	Text("Render");

	if (rendering_animation) {

		if (Button("Cancel")) {
			quit = true;
			rendering_animation = false;
		}

		SameLine();
		if (method == Method::path_trace || method == Method::software_raster) {
			ProgressBar((static_cast<float>(next_frame) + render_progress) / (max_frame + 1));
		} else {
			ProgressBar(static_cast<float>(next_frame) / (max_frame + 1));
		}

	} else {

		if (Button("Start Render")) {
			rendering_animation = true;
			has_rendered = true;
			max_frame = last_frame;
			next_frame = 0;
			folder = std::string(output_path_buffer);

			auto report_callback = [&](auto&& report) {
				std::lock_guard<std::mutex> lock(report_mut);
				if (report.first > render_progress) {
					render_progress = report.first;
					display_hdr = std::move(report.second);
					update_display = true;
				}
			};

			if(!render_cam.expired()) {
				if (method == Method::path_trace) {
					pathtracer.render(scene, render_cam.lock(), std::move(report_callback), &quit);
				} else if(method == Method::software_raster) {
					rasterizer.reset(new Rasterizer(scene, *render_cam.lock(), std::move(report_callback)));
				}
			}
		}
	}

	display_output();

	End();
}

bool Widget_Render::ui_render(Scene& scene, Manager& manager, Undo& undo, View_3D& gui_cam,
                              std::string& err) {

	using namespace ImGui;

	bool ret = false;
	if (!render_window) return ret;

	begin_window(scene, undo, manager, gui_cam);

	Separator();
	Text("Render");

	auto report_callback = [this](auto&& report) {
		std::lock_guard<std::mutex> lock(report_mut);
		if (report.first > render_progress) {
			render_progress = report.first;
			display_hdr = std::move(report.second);
			update_display = true;
			rebuild_ray_log = true;
		}
	};

	if (pathtracer.in_progress() || (rasterizer && rasterizer->in_progress()) ) {

		if (Button("Cancel")) {
			if (rasterizer) rasterizer->signal();
			quit = true;
		}

		SameLine();
		ProgressBar(render_progress);

	} else {

		if (Button("Start Render") && !render_cam.expired() && !render_cam.lock()->camera.expired()) {

			ret = true;
			quit = false;
			render_progress = 0.0f;

			if (method == Method::path_trace) {

				has_rendered = true;
				rebuild_ray_log = true;
				pathtracer.use_bvh(use_bvh);
				pathtracer.render(scene, render_cam.lock(), [this, report_callback](PT::Pathtracer::Render_Report &&report){
					report_callback(std::move(report));
					rebuild_ray_log = true;
				}, &quit);

			} else if (method == Method::software_raster) {

				has_rendered = true;
				rasterizer.reset(new Rasterizer(scene, *render_cam.lock(), std::move(report_callback)));

			} else {

				has_rendered = true;
				Renderer::get().save(manager, render_cam.lock());
			}
		}
	}

	SameLine();
	if (Button("Save Image") && has_rendered) {
		char* path = nullptr;
		NFD_SaveDialog("png", nullptr, &path);
		if (path) {

			std::string spath(path);
			if (!postfix(spath, ".png")) {
				spath += ".png";
			}

			std::vector<uint8_t> data;

			if (method == Method::path_trace || method == Method::software_raster) {
				std::lock_guard<std::mutex> lock(report_mut);
				display_hdr.tonemap_to(data, exposure);
				stbi_flip_vertically_on_write(true);
			} else {
				Renderer::get().saved(data);
				stbi_flip_vertically_on_write(true);
			}

			uint32_t out_w = render_cam.lock()->camera.lock()->film.width;
			uint32_t out_h = render_cam.lock()->camera.lock()->film.height;

			if (!stbi_write_png(spath.c_str(), out_w, out_h, 4, data.data(), out_w * 4)) {
				err = "Failed to write png!";
			}
			free(path);
		}
	}

	if (method == Method::path_trace && has_rendered) {
		SameLine();
		if (Button("Add Samples") && !render_cam.expired() &&
		    !render_cam.lock()->camera.expired()) {

			quit = false;
			render_progress = 0.0f;
			pathtracer.render(scene, render_cam.lock(), std::move(report_callback), &quit, true);
		}
	}

	display_output();

	End();
	return ret;
}

void Widget_Render::display_output() {
	using namespace ImGui;

	if (!has_rendered) return;

	if (!render_cam.expired() && !render_cam.lock()->camera.expired()) {

		float avail = GetContentRegionAvail().x;

		uint32_t out_w = render_cam.lock()->camera.lock()->film.width;
		uint32_t out_h = render_cam.lock()->camera.lock()->film.height;
		float w = std::min(avail, static_cast<float>(out_w));
		float h = (w / out_w) * out_h;

		{
			std::lock_guard<std::mutex> lock(report_mut);
			if (update_display) {
				display = display_hdr.to_gl(exposure);
				update_display = false;
			}
		}

		auto to_id = [](GL::TexID id) {
			return reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(id));
		};

		if (method == Method::path_trace) {
			Image(to_id(display.get_id()), {w, h}, {0.0f, 1.0f}, {1.0f, 0.0f});

			if (!pathtracer.in_progress() && has_rendered) {
				auto [build, render] = pathtracer.completion_time();
				Text("Scene built in %.2fs, rendered in %.2fs.", build, render);
			}
		} else if (method == Method::software_raster) {
			Image(to_id(display.get_id()), {w, h}, {0.0f, 1.0f}, {1.0f, 0.0f});

			if (rasterizer && !rasterizer->in_progress() && has_rendered) {
				Text("Scene rendered in %.2fs.", rasterizer->completion_time);
			}
		} else {
			Image(to_id(Renderer::get().saved()), {w, h}, {0.0f, 1.0f}, {1.0f, 0.0f});
		}
	}
}

std::string Widget_Render::step_animation(Scene& scene, Animator& animator, Manager& manager) {

	if (rendering_animation) {

		if (next_frame == max_frame) {
			rendering_animation = false;
			return {};
		}
		if (folder.empty()) {
			rendering_animation = false;
			return "No output folder!";
		}

		//helper called when scene needs to be advanced (not actually called every 'step_animation' step, since pathtrace / software rasterize need to wait)
		auto step_frame = [&,this]() {
			//animate scene:
			Scene::StepOpts opts;
			opts.use_bvh = use_bvh;
			//TODO: opts.thread_pool = &thread_pool;

			if (next_frame == 0) opts.reset = true;
			scene.step(animator, float(next_frame) - 1.0f, float(next_frame), 1.0f / animator.frame_rate, opts);

			//for hardware_raster, need to invalidate_gpu on skinned meshes (since it uses these to render and the scene update may have broken them)
			if (method == Method::hardware_raster) {
				for(auto& [name, _] : scene.skinned_meshes) {
					manager.invalidate_gpu(name);
				}
			}
		};

		auto camera = render_cam.lock()->camera.lock();

		auto report_callback = [&](auto&& report) {
			std::lock_guard<std::mutex> lock(report_mut);
			render_progress = report.first;
			display_hdr = std::move(report.second);
			update_display = true;
		};

		if (method == Method::hardware_raster) {
			step_frame();

			std::vector<unsigned char> data;

			Renderer::get().save(manager, render_cam.lock());
			Renderer::get().saved(data);

			std::stringstream str;
			str << std::setfill('0') << std::setw(4) << next_frame;
#ifdef _WIN32
			std::string path = folder + "\\" + str.str() + ".png";
#else
			std::string path = folder + "/" + str.str() + ".png";
#endif

			stbi_flip_vertically_on_write(true);
			if (!stbi_write_png(path.c_str(), camera->film.width, camera->film.height, 4,
			                    data.data(), camera->film.width * 4)) {
				rendering_animation = false;
				return "Failed to write output!";
			}

			next_frame++;

		} else if (method == Method::path_trace) {

			if (!pathtracer.in_progress()) {
				step_frame();

				std::vector<unsigned char> data;

				{
					std::lock_guard<std::mutex> lock(report_mut);
					display_hdr.tonemap_to(data, exposure);
				}
				std::stringstream str;
				str << std::setfill('0') << std::setw(4) << next_frame;
#ifdef _WIN32
				std::string path = folder + "\\" + str.str() + ".png";
#else
				std::string path = folder + "/" + str.str() + ".png";
#endif

				stbi_flip_vertically_on_write(true);
				if (!stbi_write_png(path.c_str(), camera->film.width, camera->film.height, 4,
				                    data.data(), camera->film.width * 4)) {
					rendering_animation = false;
					return "Failed to write output!";
				}

				render_progress = 0.0f;
				pathtracer.use_bvh(use_bvh);
				pathtracer.render(scene, render_cam.lock(), std::move(report_callback), &quit);
				next_frame++;
			}

		} else if (method == Method::software_raster) {

			if (!(rasterizer && rasterizer->in_progress())) {
				step_frame();

				std::vector<unsigned char> data;

				{
					std::lock_guard<std::mutex> lock(report_mut);
					display_hdr.tonemap_to(data, exposure);
				}
				std::stringstream str;
				str << std::setfill('0') << std::setw(4) << next_frame;
#ifdef _WIN32
				std::string path = folder + "\\" + str.str() + ".png";
#else
				std::string path = folder + "/" + str.str() + ".png";
#endif

				stbi_flip_vertically_on_write(true);
				if (!stbi_write_png(path.c_str(), camera->film.width, camera->film.height, 4,
				                    data.data(), camera->film.width * 4)) {
					rendering_animation = false;
					return "Failed to write output!";
				}

				render_progress = 0.0f;
				rasterizer.reset(new Rasterizer(scene, *render_cam.lock(), std::move(report_callback)));
				next_frame++;
			}
		}
	}
	return {};
}

PT::Pathtracer& Widget_Render::tracer() {
	return pathtracer;
}

void Widget_Render::render_log(const Mat4& view) {
	if (rebuild_ray_log) {
		rebuild_ray_log = false;
		ray_log.clear();
		for (auto& l : pathtracer.copy_ray_log()) {
			ray_log.add(l.ray.point, l.ray.at(l.t), l.color);
		}
	}
	Renderer::get().lines(ray_log, view);
}

} // namespace Gui
