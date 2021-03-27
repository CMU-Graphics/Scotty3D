
#pragma once

#include <variant>

#include "../lib/mathlib.h"
#include "../lib/spectrum.h"
#include "../util/hdr_image.h"

#include "light.h"
#include "samplers.h"

namespace PT {

struct Env_Hemisphere {

    Env_Hemisphere(Spectrum r) : radiance(r) {
    }

    Light_Sample sample() const;
    Spectrum sample_direction(Vec3 dir) const;

    Spectrum radiance;
    Samplers::Hemisphere::Uniform sampler;
};

struct Env_Sphere {

    Env_Sphere(Spectrum r) : radiance(r) {
    }

    Light_Sample sample() const;
    Spectrum sample_direction(Vec3 dir) const;

    Spectrum radiance;
    Samplers::Sphere::Uniform sampler;
};

struct Env_Map {

    Env_Map(HDR_Image&& img) : image(std::move(img)), sampler(image) {
    }

    Light_Sample sample() const;
    Spectrum sample_direction(Vec3 dir) const;

    HDR_Image image;
    Samplers::Sphere::Image sampler;
};

class Env_Light {
public:
    Env_Light(Env_Hemisphere&& l) : underlying(std::move(l)) {
    }
    Env_Light(Env_Sphere&& l) : underlying(std::move(l)) {
    }
    Env_Light(Env_Map&& l) : underlying(std::move(l)) {
    }

    Env_Light(const Env_Light& src) = delete;
    Env_Light& operator=(const Env_Light& src) = delete;
    Env_Light& operator=(Env_Light&& src) = default;
    Env_Light(Env_Light&& src) = default;

    Light_Sample sample(Vec3) const {
        return std::visit(overloaded{[](const Env_Hemisphere& h) { return h.sample(); },
                                     [](const Env_Sphere& h) { return h.sample(); },
                                     [](const Env_Map& h) { return h.sample(); }},
                          underlying);
    }

    Spectrum sample_direction(Vec3 dir) const {
        return std::visit(
            overloaded{[&dir](const Env_Hemisphere& h) { return h.sample_direction(dir); },
                       [&dir](const Env_Sphere& h) { return h.sample_direction(dir); },
                       [&dir](const Env_Map& h) { return h.sample_direction(dir); }},
            underlying);
    }

    bool is_discrete() const {
        return false;
    }

private:
    std::variant<Env_Hemisphere, Env_Sphere, Env_Map> underlying;
};

} // namespace PT