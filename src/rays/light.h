
#pragma once

#include <variant>

#include "../lib/mathlib.h"
#include "../lib/spectrum.h"
#include "../scene/object.h"
#include "../util/hdr_image.h"

#include "samplers.h"

namespace PT {

struct Light_Sample {

    Spectrum radiance;
    Vec3 direction;
    float distance;
    float pdf;

    void transform(const Mat4& T) {
        direction = T.rotate(direction);
    }
};

struct Directional_Light {

    Directional_Light(Spectrum r) : radiance(r), sampler(Vec3(0.0f, 1.0f, 0.0f)) {
    }

    Light_Sample sample(Vec3 from) const;

    Spectrum radiance;
    Samplers::Direction sampler;
};

struct Point_Light {

    Point_Light(Spectrum r) : radiance(r), sampler(Vec3(0.0f)) {
    }

    Light_Sample sample(Vec3 from) const;

    Spectrum radiance;
    Samplers::Point sampler;
};

struct Spot_Light {

    Spot_Light(Spectrum r, Vec2 a) : radiance(r), angle_bounds(a), sampler(Vec3(0.0f)) {
    }

    Light_Sample sample(Vec3 from) const;

    Spectrum radiance;
    Vec2 angle_bounds;
    Samplers::Point sampler;
};

struct Rect_Light {

    Rect_Light(Spectrum r, Vec2 s) : radiance(r), size(s), sampler(size) {
    }

    Light_Sample sample(Vec3 from) const;

    Spectrum radiance;
    Vec2 size;
    Samplers::Rect::Uniform sampler;
};

class Light {
public:
    Light(Directional_Light&& l, Scene_ID id, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), underlying(std::move(l)) {
        has_trans = trans != Mat4::I;
    }
    Light(Point_Light&& l, Scene_ID id, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), underlying(std::move(l)) {
        has_trans = trans != Mat4::I;
    }
    Light(Spot_Light&& l, Scene_ID id, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), underlying(std::move(l)) {
        has_trans = trans != Mat4::I;
    }
    Light(Rect_Light&& l, Scene_ID id, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), underlying(std::move(l)) {
        has_trans = trans != Mat4::I;
    }

    Light(const Light& src) = delete;
    Light& operator=(const Light& src) = delete;
    Light& operator=(Light&& src) = default;
    Light(Light&& src) = default;

    Light_Sample sample(Vec3 from) const {
        if(has_trans) from = itrans * from;
        Light_Sample ret =
            std::visit(overloaded{[&from](const auto& l) { return l.sample(from); }}, underlying);
        if(has_trans) ret.transform(trans);
        return ret;
    }

    bool is_discrete() const {
        return std::visit(overloaded{[](const Directional_Light&) { return true; },
                                     [](const Point_Light&) { return true; },
                                     [](const Spot_Light&) { return true; },
                                     [](const Rect_Light&) { return false; }},
                          underlying);
    }

    Scene_ID id() const {
        return _id;
    }
    void set_trans(const Mat4& T) {
        trans = T;
        itrans = T.inverse();
        has_trans = trans != Mat4::I;
    }

private:
    bool has_trans;
    Mat4 trans, itrans;
    Scene_ID _id;
    std::variant<Directional_Light, Point_Light, Spot_Light, Rect_Light> underlying;
};

} // namespace PT
