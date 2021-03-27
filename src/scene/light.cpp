
#include "light.h"

#include "../geometry/util.h"
#include "renderer.h"

#include <sstream>

const char* Light_Type_Names[(int)Light_Type::count] = {"Directional", "Sphere", "Hemisphere",
                                                        "Point",       "Spot",   "Rectangle"};

Scene_Light::Scene_Light(Light_Type type, Scene_ID id, Pose p, std::string n)
    : pose(p), _id(id), _lines(1.0f) {
    opt.type = type;
    if(n.size()) {
        snprintf(opt.name, max_name_len, "%s", n.c_str());
    } else {
        snprintf(opt.name, max_name_len, "%s Light %d", Light_Type_Names[(int)type], id);
    }
}

bool Scene_Light::is_env() const {
    return opt.type == Light_Type::sphere || opt.type == Light_Type::hemisphere;
}

Scene_ID Scene_Light::id() const {
    return _id;
}

void Scene_Light::set_time(float time) {
    if(lanim.splines.any()) {
        lanim.at(time, opt);
    }
    if(anim.splines.any()) {
        pose = anim.at(time);
    }
    dirty();
}

void Scene_Light::emissive_clear() {
    opt.has_emissive_map = false;
}

HDR_Image Scene_Light::emissive_copy() const {
    return _emissive.copy();
}

std::string Scene_Light::emissive_load(std::string file) {
    std::string err = _emissive.load_from(file);
    if(err.empty()) {
        opt.has_emissive_map = true;
    }
    return err;
}

std::string Scene_Light::emissive_loaded() const {
    return _emissive.loaded_from();
}

const GL::Tex2D& Scene_Light::emissive_texture() const {
    return _emissive.get_texture();
}

BBox Scene_Light::bbox() const {
    BBox box = _mesh.bbox();
    box.transform(pose.transform());
    return box;
}

Scene_Light::Scene_Light(Scene_Light&& src) : _lines(1.0f) {
    *this = std::move(src);
}

void Scene_Light::regen_mesh() {

    switch(opt.type) {
    case Light_Type::spot: {
        Vec3 col(opt.spectrum.r, opt.spectrum.g, opt.spectrum.b);
        _lines = Util::spotlight_mesh(col, opt.angle_bounds.x, opt.angle_bounds.y);
        _mesh = Util::sphere_mesh(0.15f, 2);
    } break;
    case Light_Type::directional: {
        _mesh = Util::arrow_mesh(0.03f, 0.075f, 1.0f);
    } break;
    case Light_Type::point: {
        _mesh = Util::sphere_mesh(0.15f, 2);
    } break;
    case Light_Type::rectangle: {
        _mesh = Util::quad_mesh(opt.size.x, opt.size.y);
    } break;
    default: break;
    }

    _dirty = false;
}

void Scene_Light::dirty() {
    _dirty = true;
}

Spectrum Scene_Light::radiance() const {
    return opt.spectrum * opt.intensity;
}

void Scene_Light::render(const Mat4& view, bool depth_only, bool posed) {

    if(_dirty) regen_mesh();

    Renderer& renderer = Renderer::get();

    Spectrum s = opt.spectrum;
    s.make_srgb();
    Vec3 col(s.r, s.g, s.b);

    Mat4 rot = view;
    rot.cols[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

    Mat4 T = posed ? pose.transform() : Mat4::I;

    if(opt.type == Light_Type::spot && !depth_only) renderer.lines(_lines, view, T);
    if(opt.type == Light_Type::hemisphere) {
        renderer.skydome(rot, col, 0.0f);
    } else if(opt.type == Light_Type::sphere) {
        if(opt.has_emissive_map)
            renderer.skydome(rot, col, -1.1f, _emissive.get_texture());
        else
            renderer.skydome(rot, col, -1.1f);
    } else {
        Renderer::MeshOpt opts;
        opts.modelview = view * T;
        opts.id = _id;
        opts.solid_color = true;
        opts.depth_only = depth_only;
        opts.color = col;
        renderer.mesh(_mesh, opts);
    }
}

bool operator!=(const Scene_Light::Options& l, const Scene_Light::Options& r) {
    return l.type != r.type || std::string(l.name) != std::string(r.name) ||
           l.spectrum != r.spectrum || l.intensity != r.intensity ||
           l.angle_bounds != r.angle_bounds || l.size != r.size ||
           l.has_emissive_map != r.has_emissive_map;
}

void Scene_Light::Anim_Light::at(float t, Options& o) const {
    auto [s, i, a, sz] = splines.at(t);
    o.spectrum = s;
    o.intensity = i;
    o.angle_bounds = a;
    o.size = sz;
}

void Scene_Light::Anim_Light::set(float t, Options o) {
    splines.set(t, o.spectrum, o.intensity, o.angle_bounds, o.size);
}
