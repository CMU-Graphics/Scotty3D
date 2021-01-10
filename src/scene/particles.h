
#pragma once

#include <vector>

#include "../lib/mathlib.h"
#include "../platform/gl.h"

#include "object.h"
#include "pose.h"

namespace PT {
template<typename T> class BVH;
class Object;
} // namespace PT

struct Particle {

    Vec3 pos;
    Vec3 velocity;
    float age;

    static const inline Vec3 acceleration = Vec3{0.0f, -9.8f, 0.0f};

    bool update(const PT::BVH<PT::Object>& scene, float dt, float radius);
};

class Scene_Particles {
public:
    Scene_Particles(Scene_ID id);
    Scene_Particles(Scene_ID id, Pose p, std::string name);
    Scene_Particles(Scene_ID id, GL::Mesh&& mesh);
    Scene_Particles(Scene_Particles&& src) = default;
    Scene_Particles(const Scene_Particles& src) = delete;
    ~Scene_Particles() = default;

    void operator=(const Scene_Particles& src) = delete;
    Scene_Particles& operator=(Scene_Particles&& src) = default;

    void clear();
    void step(const PT::BVH<PT::Object>& scene, float dt);
    const std::vector<Particle>& get_particles() const;

    BBox bbox() const;
    void render(const Mat4& view, bool depth_only = false, bool posed = true, bool particles_only = false);
    Scene_ID id() const;
    void set_time(float time);

    const GL::Mesh& mesh() const;
    void take_mesh(GL::Mesh&& mesh);

    static const inline int max_name_len = 256;
    struct Options {
        char name[max_name_len] = {};
        Spectrum color = Spectrum(1.0f);
        float velocity = 25.0f;
        float angle = 0.0f;
        float scale = 0.1f;
        float lifetime = 15.0f;
        float pps = 5.0f;
        bool enabled = false;
    };

    struct Anim_Particles {
        void at(float t, Options& o) const;
        void set(float t, Options o);
        Splines<Spectrum, float, float, float, float, float, bool> splines;
    };

    Options opt;
    Pose pose;
    Anim_Pose anim;
    Anim_Particles panim;

private:
    void get_r();
    Scene_ID _id;
    std::vector<Particle> particles;
    GL::Instances particle_instances;
    GL::Mesh arrow;

    float radius = 0.0f;
    double particle_cooldown = 0.0f;
};

bool operator!=(const Scene_Particles::Options& l, const Scene_Particles::Options& r);
