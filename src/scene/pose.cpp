
#include "pose.h"

Mat4 Pose::transform() const {
    return Mat4::translate(pos) * rotation_mat() * Mat4::scale(scale);
}

Mat4 Pose::rotation_mat() const {
    return Mat4::euler(euler);
}

Quat Pose::rotation_quat() const {
    return Quat::euler(euler);
}

bool Pose::valid() const {
    return pos.valid() && euler.valid() && scale.valid();
}

void Pose::clamp_euler() {
    if(!valid()) return;
    while(euler.x < 0) euler.x += 360.0f;
    while(euler.x >= 360.0f) euler.x -= 360.0f;
    while(euler.y < 0) euler.y += 360.0f;
    while(euler.y >= 360.0f) euler.y -= 360.0f;
    while(euler.z < 0) euler.z += 360.0f;
    while(euler.z >= 360.0f) euler.z -= 360.0f;
}

Pose Pose::rotated(Vec3 angles) {
    return Pose{Vec3{}, angles, Vec3{1.0f, 1.0f, 1.0f}};
}

Pose Pose::moved(Vec3 t) {
    return Pose{t, Vec3{}, Vec3{1.0f, 1.0f, 1.0f}};
}

Pose Pose::scaled(Vec3 s) {
    return Pose{Vec3{}, Vec3{}, s};
}

Pose Pose::id() {
    return Pose{Vec3{}, Vec3{}, Vec3{1.0f, 1.0f, 1.0f}};
}

bool operator==(const Pose& l, const Pose& r) {
    return l.pos == r.pos && l.euler == r.euler && l.scale == r.scale;
}

bool operator!=(const Pose& l, const Pose& r) {
    return l.pos != r.pos || l.euler != r.euler || l.scale != r.scale;
}

Pose Anim_Pose::at(float t) const {
    auto [p, r, s] = splines.at(t);
    return Pose{p, r.to_euler(), s};
}

void Anim_Pose::set(float t, Pose p) {
    splines.set(t, p.pos, Quat::euler(p.euler), p.scale);
}
