
#include "material.h"

const char* Material_Type_Names[(int)Material_Type::count] = {"Lambertian", "Mirror", "Refract",
                                                              "Glass", "Diffuse Light"};

bool operator!=(const Material::Options& l, const Material::Options& r) {
    return l.albedo != r.albedo || l.emissive != r.emissive || l.ior != r.ior ||
           l.reflectance != r.reflectance || l.transmittance != r.transmittance ||
           l.type != r.type || l.intensity != r.intensity;
}

Vec3 Material::layout_color() const {
    Vec3 color;
    switch(opt.type) {
    case Material_Type::lambertian: {
        color = opt.albedo.to_vec();
    } break;
    case Material_Type::mirror: {
        color = opt.reflectance.to_vec();
    } break;
    case Material_Type::refract:
    case Material_Type::glass: {
        color = opt.transmittance.to_vec();
    } break;
    case Material_Type::diffuse_light: {
        color = opt.emissive.to_vec();
    } break;
    default: break;
    }
    return color;
}

Spectrum Material::emissive() const {
    return opt.emissive * opt.intensity;
}

Material Material::copy() const {
    Material ret;
    ret.opt = opt;
    return ret;
}

void Material::Anim_Material::at(float t, Material::Options& o) const {
    auto [a, r, tr, e, in, ior] = splines.at(t);
    o.albedo = a;
    o.reflectance = r;
    o.transmittance = tr;
    o.emissive = e;
    o.intensity = in;
    o.ior = ior;
}

void Material::Anim_Material::set(float t, Material::Options o) {
    splines.set(t, o.albedo, o.reflectance, o.transmittance, o.emissive, o.intensity, o.ior);
}
