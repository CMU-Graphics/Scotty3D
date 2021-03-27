
#pragma once

#include <variant>

#include "../lib/bbox.h"
#include "../platform/gl.h"
#include "scene.h"

namespace Gui {
class Model;
}

// Singleton
class Renderer {
public:
    static void setup(Vec2 dim);
    static void shutdown();
    static Renderer& get();

    void begin();
    void complete();
    void reset_depth();

    void proj(const Mat4& proj);
    void update_dim(Vec2 dim);
    void settings_gui(bool* open);
    void set_samples(int samples);
    unsigned int read_id(Vec2 pos);

    struct MeshOpt {
        unsigned int id;
        Mat4 modelview;
        Vec3 color, sel_color, hov_color;
        unsigned int sel_id = 0, hov_id = 0;
        float alpha = 1.0f;
        bool wireframe = false;
        bool solid_color = false;
        bool depth_only = false;
        bool per_vert_id = false;
    };

    struct HalfedgeOpt {
        HalfedgeOpt(Gui::Model& e) : editor(e) {
        }
        Gui::Model& editor;
        Mat4 modelview;
        Vec3 f_color = Vec3{1.0f};
        Vec3 v_color = Vec3{1.0f};
        Vec3 e_color = Vec3{0.8f};
        Vec3 he_color = Vec3{0.6f};
        Vec3 err_color = Vec3{1.0f, 0.0f, 0.0f};
        unsigned int err_id = 0;
    };

    // NOTE(max): updates & uses the indices in mesh for selection/traversal
    void halfedge_editor(HalfedgeOpt opt);
    void mesh(GL::Mesh& mesh, MeshOpt opt);
    void lines(const GL::Lines& lines, const Mat4& view, const Mat4& model = Mat4::I,
               float alpha = 1.0f);
    void instances(Renderer::MeshOpt opt, GL::Instances& inst);

    void outline(const Mat4& view, Scene_Item& obj);
    void begin_outline();
    void end_outline(const Mat4& view, BBox box);

    void skydome(const Mat4& rotation, Vec3 color, float cosine);
    void skydome(const Mat4& rotation, Vec3 color, float cosine, const GL::Tex2D& tex);

    void sphere(MeshOpt opt);
    void capsule(MeshOpt opt, float height, float rad);
    void capsule(MeshOpt opt, const Mat4& mdl, float height, float rad, BBox& box);

    GLuint saved() const;
    void saved(std::vector<unsigned char>& data) const;
    void save(Scene& scene, const Camera& cam, int w, int h, int samples);

private:
    Renderer(Vec2 dim);
    ~Renderer();
    static inline Renderer* data = nullptr;

    GL::Framebuffer framebuffer, id_resolve, save_buffer, save_output;
    GL::Shader mesh_shader, line_shader, inst_shader, dome_shader;
    GL::Mesh _sphere, _cyl, _hemi;

    int samples;
    Vec2 window_dim;
    GLubyte* id_buffer;

    Mat4 _proj;
};
