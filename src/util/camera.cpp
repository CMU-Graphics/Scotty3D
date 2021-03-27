
#include "camera.h"

const Vec3 UP = Vec3{0.0f, 1.0f, 0.0f};

Camera::Camera(Vec2 dim) {
    reset();
    set_ar(dim);
}

Mat4 Camera::get_view() const {
    return view;
}

Mat4 Camera::get_proj() const {
    return Mat4::project(vert_fov, aspect_ratio, near_plane);
}

Vec3 Camera::pos() const {
    return position;
}

Vec3 Camera::front() const {
    return (looking_at - position).unit();
}

float Camera::dist() const {
    return (position - looking_at).norm();
}

void Camera::look_at(Vec3 cent, Vec3 pos) {
    position = pos;
    looking_at = cent;
    radius = (pos - cent).norm();
    if(dot(front(), UP) == -1.0f)
        rot = Quat::euler(Vec3{270.0f, 0.0f, 0.0f});
    else
        rot = Quat::euler(Mat4::rotate_z_to(front()).to_euler());
    update_pos();
}

void Camera::reset() {
    vert_fov = 90.0f;
    aspect_ratio = 1.7778f;
    rot = Quat::euler(Vec3(-45.0f, 45.0f, 0.0f));
    near_plane = 0.01f;
    radius = 5.0f;
    radius_sens = 0.25f;
    move_sens = 0.015f;
    orbit_sens = 0.2f;
    aperture = 0.0f;
    focal_dist = 1.0f;
    looking_at = Vec3();
    update_pos();
}

void Camera::mouse_orbit(Vec2 off) {
    float up_rot = -off.x * orbit_sens;
    float right_rot = off.y * orbit_sens;

    Vec3 up = rot.rotate(UP);
    Vec3 f = front();
    Vec3 right = cross(f, up).unit();

    rot = Quat::axis_angle(UP, up_rot) * Quat::axis_angle(right, right_rot) * rot;
    update_pos();
}

void Camera::mouse_move(Vec2 off) {
    Vec3 up = rot.rotate(UP);
    Vec3 f = front();
    Vec3 right = cross(f, up).unit();

    looking_at += -right * off.x * move_sens + up * off.y * move_sens;
    update_pos();
}

void Camera::mouse_radius(float off) {
    radius -= off * radius_sens;
    radius = std::max(radius, 2.0f * near_plane);
    update_pos();
}

void Camera::set_fov(float f) {
    vert_fov = f;
}

float Camera::get_h_fov() const {
    float vfov = Radians(vert_fov);
    float hfov = 2.0f * std::atan(aspect_ratio * std::tan(vfov / 2.0f));
    return Degrees(hfov);
}

float Camera::get_fov() const {
    return vert_fov;
}

float Camera::get_ar() const {
    return aspect_ratio;
}

float Camera::get_near() const {
    return near_plane;
}

Vec3 Camera::center() const {
    return looking_at;
}

void Camera::set_ar(float a) {
    aspect_ratio = a;
}

void Camera::set_ar(Vec2 dim) {
    aspect_ratio = dim.x / dim.y;
}

void Camera::set_ap(float ap) {
    aperture = ap;
}

float Camera::get_ap() const {
    return aperture;
}

void Camera::set_dist(float dist) {
    focal_dist = dist;
}

float Camera::get_dist() const {
    return focal_dist;
}

void Camera::update_pos() {
    position = rot.rotate(Vec3{0.0f, 0.0f, 1.0f});
    position = looking_at + radius * position.unit();
    iview = Mat4::translate(position) * rot.to_mat();
    view = iview.inverse();
}
