
#include "viewer.h"

const Vec3 UP = Vec3{0.0f, 1.0f, 0.0f};

View_3D::View_3D() {
	reset();
}

View_3D::View_3D(Vec2 dim) {
	reset();
	set_ar(dim);
}

Mat4 View_3D::get_view() const {
	return view;
}

Mat4 View_3D::get_proj() const {
	return Mat4::perspective(vert_fov, aspect_ratio, near_plane);
}

Vec3 View_3D::pos() const {
	return position;
}

Vec3 View_3D::front() const {
	return (looking_at - position).unit();
}

float View_3D::dist() const {
	return (position - looking_at).norm();
}

void View_3D::look_at(Vec3 cent, Vec3 pos) {
	position = pos;
	looking_at = cent;
	radius = (pos - cent).norm();
	if (dot(front(), UP) == -1.0f)
		rot = Quat::euler(Vec3{270.0f, 0.0f, 0.0f});
	else
		rot = Quat::euler(Mat4::rotate_z_to(front()).to_euler());
	update_pos();
}

void View_3D::reset() {
	vert_fov = 90.0f;
	aspect_ratio = 1.7778f;
	rot = Quat::euler(Vec3(-45.0f, 45.0f, 0.0f));
	near_plane = 0.01f;
	radius = 5.0f;
	radius_sens = 0.25f;
	move_sens = 0.005f;
	orbit_sens = 0.2f;
	aperture = 0.0f;
	focal_dist = 1.0f;
	looking_at = Vec3();
	update_pos();
}

void View_3D::mouse_orbit(Vec2 off) {
	float up_rot = -off.x * orbit_sens;
	float right_rot = -off.y * orbit_sens;

	if (orbit_flip_vertical) {
		right_rot = -right_rot;
	}

	Vec3 up = rot.rotate(UP);
	Vec3 f = front();
	Vec3 right = cross(f, up).unit();

	rot = Quat::axis_angle(UP, up_rot) * Quat::axis_angle(right, right_rot) * rot;
	update_pos();
}

void View_3D::mouse_move(Vec2 off) {
	Vec3 up = rot.rotate(UP);
	Vec3 f = front();
	Vec3 right = cross(f, up).unit();

	float scale = move_sens * radius;
	looking_at += -right * off.x * scale + up * off.y * scale;
	update_pos();
}

void View_3D::mouse_radius(float off) {
	radius -= off * radius_sens;
	radius = std::max(radius, 2.0f * near_plane);
	update_pos();
}

void View_3D::set_fov(float f) {
	vert_fov = f;
}

float View_3D::get_h_fov() const {
	float vfov = Radians(vert_fov);
	float hfov = 2.0f * std::atan(aspect_ratio * std::tan(vfov / 2.0f));
	return Degrees(hfov);
}

float View_3D::get_fov() const {
	return vert_fov;
}

float View_3D::get_ar() const {
	return aspect_ratio;
}

float View_3D::get_near() const {
	return near_plane;
}

Vec3 View_3D::center() const {
	return looking_at;
}

void View_3D::set_ar(float a) {
	aspect_ratio = a;
}

void View_3D::set_ar(Vec2 dim) {
	aspect_ratio = dim.x / dim.y;
}

void View_3D::set_ap(float ap) {
	aperture = ap;
}

float View_3D::get_ap() const {
	return aperture;
}

void View_3D::set_dist(float dist) {
	focal_dist = dist;
}

float View_3D::get_dist() const {
	return focal_dist;
}

void View_3D::update_pos() {
	position = rot.rotate(Vec3{0.0f, 0.0f, 1.0f});
	position = looking_at + radius * position.unit();
	iview = Mat4::translate(position) * rot.to_mat();
	view = iview.inverse();
}
