
#include "light.h"

namespace PT {

Light_Sample Directional_Light::sample(Vec3) const {
    Light_Sample ret;
    ret.direction = Vec3(0.0f, -1.0f, 0.0f);
    ret.distance = std::numeric_limits<float>::infinity();
    ret.pdf = 1.0f;
    ret.radiance = radiance;
    return ret;
}

Light_Sample Point_Light::sample(Vec3 from) const {
    Light_Sample ret;
    ret.direction = -from.unit();
    ret.distance = from.norm();
    ret.pdf = 1.0f;
    ret.radiance = radiance;
    return ret;
}

Light_Sample Spot_Light::sample(Vec3 from) const {
    Light_Sample ret;
    float angle = std::atan2(Vec2(from.x, from.z).norm(), from.y);
    angle = std::abs(Degrees(angle));
    ret.direction = -from.unit();
    ret.distance = from.norm();
    ret.pdf = 1.0f;
    ret.radiance =
        (1.0f - smoothstep(angle_bounds.x / 2.0f, angle_bounds.y / 2.0f, angle)) * radiance;
    return ret;
}

Light_Sample Rect_Light::sample(Vec3 from) const {
    Light_Sample ret;

    Vec2 sample = sampler.sample(ret.pdf);
    Vec3 point(sample.x - size.x / 2.0f, 0.0f, sample.y - size.y / 2.0f);
    Vec3 dir = point - from;

    float cos_theta = dir.y;
    float squared_dist = dir.norm_squared();
    float dist = std::sqrt(squared_dist);

    ret.direction = dir / dist;
    ret.distance = dist;
    ret.pdf *= squared_dist / std::abs(cos_theta);
    ret.radiance = cos_theta > 0.0f ? radiance : Spectrum{};
    return ret;
}

} // namespace PT
