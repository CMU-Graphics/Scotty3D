
#pragma once

#include "../lib/mathlib.h"
#include "../util/hdr_image.h"

namespace Samplers {

// These samplers are discrete. Note they output a probability _mass_ function
struct Point {
    Point(Vec3 point) : point(point) {
    }

    Vec3 sample(float& pmf) const;
    Vec3 point;
};

struct Two_Points {
    Two_Points(Vec3 p1, Vec3 p2, float p_p1) : p1(p1), p2(p2), prob(p_p1) {
    }

    Vec3 sample(float& pmf) const;
    Vec3 p1, p2;
    float prob;
};

using Direction = Point;
using Two_Directions = Two_Points;

// These are continuous. Note they output a probabilty _density_ function
namespace Rect {

struct Uniform {
    Uniform(Vec2 size = Vec2(1.0f)) : size(size) {
    }

    Vec2 sample(float& pdf) const;
    Vec2 size;
};

} // namespace Rect

namespace Hemisphere {

struct Uniform {
    Uniform() = default;
    Vec3 sample(float& pdf) const;
};

struct Cosine {
    Cosine() = default;
    Vec3 sample(float& pdf) const;
};
} // namespace Hemisphere

namespace Sphere {

struct Uniform {
    Uniform() = default;
    Vec3 sample(float& pdf) const;
    Hemisphere::Uniform hemi;
};

struct Image {
    Image(const HDR_Image& image);
    Vec3 sample(float& pdf) const;

    size_t w = 0, h = 0;
    std::vector<float> pdf, cdf;
    float total = 0.0f;
};

} // namespace Sphere
} // namespace Samplers
