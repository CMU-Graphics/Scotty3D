
#include "camera.h"

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
    update_dirs();
}

void Camera::reset() {
    vert_fov = 90.0f;
    aspect_ratio = 1.7778f;
    pitch = -45.0f;
    yaw = 45.0f;
    near_plane = 0.01f;
    radius = 5.0f;
    radius_sens = 0.25f;
    move_sens = 0.015f;
    orbit_sens = 0.2f;
    looking_at = Vec3();
    global_up = Vec3(0, 1, 0);
    update_pos();
}

void Camera::mouse_orbit(Vec2 off) {
    yaw += off.x * orbit_sens;
    pitch += off.y * orbit_sens;
    if (yaw > 360.0f) yaw = 0.0f;
    else if (yaw < 0.0f) yaw = 360.0f;
    pitch = clamp(pitch, -89.5f, 89.5f);
    update_pos();
}

void Camera::mouse_move(Vec2 off) {
    Vec3 front = (position - looking_at).unit();
    Vec3 right = cross(front, global_up).unit();
    Vec3 up = cross(front, right).unit();
    looking_at += right * off.x * move_sens - up * off.y * move_sens;
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

void Camera::update_dirs() {
    Vec3 dir = front();
    yaw = Degrees(std::atan2(dir.z, dir.x));
    pitch = Degrees(std::atan2(dir.y, Vec2(dir.x, dir.z).norm()));
    while (yaw > 360.0f) yaw -= 360.0f;
    while (yaw < 0.0f) yaw += 360.0f;
    pitch = clamp(pitch, -89.5f, 89.5f);
    update_pos();
}

void Camera::update_pos() {
    position.x = std::cos(Radians(pitch)) * std::cos(Radians(yaw));
    position.y = std::sin(Radians(pitch));
    position.z = std::sin(Radians(yaw)) * std::cos(Radians(pitch));
    position = looking_at - radius * position.unit();
    
    view = Mat4::look_at(position, looking_at, global_up);
    iview = view.inverse();
}
