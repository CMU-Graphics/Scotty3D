
#pragma once

#include "../lib/spectrum.h"

enum class Material_Type : int { lambertian, mirror, refract, glass, diffuse_light, count };
extern const char *Material_Type_Names[(int)Material_Type::count];

class Material {
public:
    Material() = default;
    Material(Material_Type type) { opt.type = type; }
    Material(const Material &src) = delete;
    Material(Material &&src) = default;
    ~Material() = default;

    void operator=(const Material &src) = delete;
    Material &operator=(Material &&src) = default;

    // TODO: decide how to handle texture resources. should probably use a texture repository
    // and also integrate that into the envmaps
    Material copy() const;

    Spectrum emissive() const;
    Vec3 layout_color() const;

    struct Options {
        Material_Type type = Material_Type::lambertian;
        Spectrum albedo = Spectrum(1.0f);
        Spectrum reflectance = Spectrum(1.0f);
        Spectrum transmittance = Spectrum(1.0f);
        Spectrum emissive = Spectrum(1.0f);
        float intensity = 1.0f;
        float ior = 1.2f;
    };

    Options opt;
};

bool operator!=(const Material::Options &l, const Material::Options &r);
