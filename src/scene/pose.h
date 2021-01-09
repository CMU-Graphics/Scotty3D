
#pragma once

#include "../geometry/spline.h"
#include "../lib/mathlib.h"
#include <set>

struct Pose {
    Vec3 pos;
    Vec3 euler;
    Vec3 scale = Vec3{1.0f};

    Mat4 transform() const;
    Mat4 rotation_mat() const;
    Quat rotation_quat() const;

    void clamp_euler();
    bool valid() const;

    static Pose rotated(Vec3 angles);
    static Pose moved(Vec3 t);
    static Pose scaled(Vec3 s);
    static Pose id();
};

bool operator==(const Pose& l, const Pose& r);
bool operator!=(const Pose& l, const Pose& r);

struct Anim_Pose {
    Pose at(float t) const;
    void set(float t, Pose p);
    Splines<Vec3, Quat, Vec3> splines;
};
