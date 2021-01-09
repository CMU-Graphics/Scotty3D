
#pragma once

#include <string>

#include "../lib/spectrum.h"
#include "../platform/gl.h"
#include "../rays/samplers.h"
#include "../util/hdr_image.h"

#include "object.h"
#include "pose.h"

enum class Light_Type : int { directional, sphere, hemisphere, point, spot, rectangle, count };
extern const char* Light_Type_Names[(int)Light_Type::count];

class Scene_Light {
public:
    Scene_Light(Light_Type type, Scene_ID id, Pose p, std::string n = {});
    Scene_Light(const Scene_Light& src) = delete;
    Scene_Light(Scene_Light&& src);
    ~Scene_Light() = default;

    void operator=(const Scene_Light& src) = delete;
    Scene_Light& operator=(Scene_Light&& src) = default;

    Scene_ID id() const;
    BBox bbox() const;

    void render(const Mat4& view, bool depth_only = false, bool posed = true);
    void dirty();

    Spectrum radiance() const;
    void set_time(float time);

    std::string emissive_load(std::string file);
    std::string emissive_loaded() const;
    HDR_Image emissive_copy() const;

    const GL::Tex2D& emissive_texture() const;
    void emissive_clear();
    bool is_env() const;

    static const inline int max_name_len = 256;
    struct Options {
        Light_Type type = Light_Type::point;
        char name[max_name_len] = {};
        Spectrum spectrum = Spectrum(1.0f);
        float intensity = 1.0f;
        bool has_emissive_map = false;
        Vec2 angle_bounds = Vec2(30.0f, 35.0f);
        Vec2 size = Vec2(1.0f);
    };

    struct Anim_Light {
        void at(float t, Options& o) const;
        void set(float t, Options o);
        Splines<Spectrum, float, Vec2, Vec2> splines;
    };

    Options opt;
    Pose pose;
    Anim_Pose anim;
    Anim_Light lanim;

private:
    void regen_mesh();

    bool _dirty = true;
    Scene_ID _id = 0;
    GL::Mesh _mesh;
    GL::Lines _lines;
    HDR_Image _emissive;
};

bool operator!=(const Scene_Light::Options& l, const Scene_Light::Options& r);
