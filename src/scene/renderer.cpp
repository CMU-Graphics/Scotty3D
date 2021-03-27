
#include <imgui/imgui.h>

#include "../geometry/util.h"
#include "../gui/manager.h"
#include "../lib/mathlib.h"

#include "renderer.h"
#include "scene.h"

static const int DEFAULT_SAMPLES = 4;

Renderer::Renderer(Vec2 dim)
    : framebuffer(2, dim, DEFAULT_SAMPLES, true), id_resolve(1, dim, 1, false),
      save_buffer(1, dim, DEFAULT_SAMPLES, true), save_output(1, dim, 1, false),
      mesh_shader(GL::Shaders::mesh_v, GL::Shaders::mesh_f),
      line_shader(GL::Shaders::line_v, GL::Shaders::line_f),
      inst_shader(GL::Shaders::inst_v, GL::Shaders::mesh_f),
      dome_shader(GL::Shaders::dome_v, GL::Shaders::dome_f), _sphere(Util::sphere_mesh(1.0f, 3)),
      _cyl(Util::cyl_mesh(1.0f, 1.0f, 64, false)), _hemi(Util::hemi_mesh(1.0f)),
      samples(DEFAULT_SAMPLES), window_dim(dim),
      id_buffer(new GLubyte[(int)dim.x * (int)dim.y * 4]) {
}

Renderer::~Renderer() {
    delete[] id_buffer;
    id_buffer = nullptr;
}

Renderer& Renderer::get() {
    assert(data);
    return *data;
}

void Renderer::setup(Vec2 dim) {
    data = new Renderer(dim);
}

void Renderer::update_dim(Vec2 dim) {

    window_dim = dim;
    delete[] id_buffer;
    id_buffer = new GLubyte[(int)dim.x * (int)dim.y * 4]();
    framebuffer.resize(dim, samples);
    save_buffer.resize(dim, save_buffer.samples());
    id_resolve.resize(dim);
    save_output.resize(dim);
}

void Renderer::shutdown() {
    delete data;
    data = nullptr;
}

void Renderer::proj(const Mat4& proj) {
    _proj = proj;
}

void Renderer::complete() {

    framebuffer.blit_to(1, id_resolve, false);

    if(!id_resolve.can_read_at()) id_resolve.read(0, id_buffer);

    framebuffer.blit_to_screen(0, window_dim);
}

void Renderer::begin() {

    framebuffer.clear(0, Vec4(Gui::Color::background, 1.0f));
    framebuffer.clear(1, Vec4{0.0f, 0.0f, 0.0f, 1.0f});
    framebuffer.clear_d();
    framebuffer.bind();
    GL::viewport(window_dim);
}

void Renderer::save(Scene& scene, const Camera& cam, int w, int h, int s) {

    Vec2 dim((float)w, (float)h);

    save_buffer.resize(dim, s);
    save_output.resize(dim);
    save_buffer.clear(0, Vec4{0.0f, 0.0f, 0.0f, 1.0f});
    save_buffer.bind();
    GL::viewport(dim);

    Mat4 view = cam.get_view();
    scene.for_items([&](Scene_Item& item) {
        if(item.is<Scene_Light>()) {
            Scene_Light& light = item.get<Scene_Light>();
            if(!light.is_env()) return;
        }

        if(item.is<Scene_Particles>()) {
            item.get<Scene_Particles>().render(view, false, true, true);
        } else {
            item.render(view);
        }
    });

    save_buffer.blit_to(0, save_output, true);

    framebuffer.bind();
    GL::viewport(window_dim);
}

void Renderer::saved(std::vector<unsigned char>& out) const {
    save_output.flush();
    out.resize(save_output.bytes());
    save_output.read(0, out.data());
}

GLuint Renderer::saved() const {
    save_output.flush();
    return save_output.get_output(0);
}

void Renderer::lines(const GL::Lines& lines, const Mat4& view, const Mat4& model, float alpha) {

    Mat4 mvp = _proj * view * model;
    line_shader.bind();
    line_shader.uniform("mvp", mvp);
    line_shader.uniform("alpha", alpha);
    lines.render(framebuffer.is_multisampled());
}

void Renderer::skydome(const Mat4& rotation, Vec3 color, float cosine, const GL::Tex2D& tex) {

    tex.bind();
    dome_shader.bind();
    dome_shader.uniform("tex", 0);
    dome_shader.uniform("use_texture", true);
    dome_shader.uniform("color", color);
    dome_shader.uniform("cosine", cosine);
    dome_shader.uniform("transform", _proj * rotation);
    _sphere.render();
}

void Renderer::skydome(const Mat4& rotation, Vec3 color, float cosine) {

    dome_shader.bind();
    dome_shader.uniform("use_texture", false);
    dome_shader.uniform("color", color);
    dome_shader.uniform("cosine", cosine);
    dome_shader.uniform("transform", _proj * rotation);
    _sphere.render();
}

void Renderer::sphere(MeshOpt opt) {
    mesh(_sphere, opt);
}

void Renderer::capsule(MeshOpt opt, const Mat4& mdl, float height, float rad, BBox& box) {

    Mat4 T = opt.modelview;
    Mat4 cyl = mdl * Mat4::scale(Vec3{rad, height, rad});
    Mat4 bot = mdl * Mat4::scale(Vec3{rad});
    Mat4 top = mdl * Mat4::translate(Vec3{0.0f, height, 0.0f}) *
               Mat4::euler(Vec3{180.0f, 0.0f, 0.0f}) * Mat4::scale(Vec3{rad});

    opt.modelview = T * cyl;
    mesh(_cyl, opt);
    opt.modelview = T * bot;
    mesh(_hemi, opt);
    opt.modelview = T * top;
    mesh(_hemi, opt);

    BBox b = _cyl.bbox();
    b.transform(cyl);
    box.enclose(b);
    b = _hemi.bbox();
    b.transform(bot);
    box.enclose(b);
    b = _hemi.bbox();
    b.transform(top);
    box.enclose(b);
}

void Renderer::capsule(MeshOpt opt, float height, float rad) {
    BBox box;
    capsule(opt, Mat4::I, height, rad, box);
}

void Renderer::mesh(GL::Mesh& mesh, Renderer::MeshOpt opt) {

    mesh_shader.bind();
    mesh_shader.uniform("use_v_id", opt.per_vert_id);
    mesh_shader.uniform("id", opt.id);
    mesh_shader.uniform("alpha", opt.alpha);
    mesh_shader.uniform("mvp", _proj * opt.modelview);
    mesh_shader.uniform("normal", Mat4::transpose(Mat4::inverse(opt.modelview)));
    mesh_shader.uniform("solid", opt.solid_color);
    mesh_shader.uniform("sel_color", opt.sel_color);
    mesh_shader.uniform("sel_id", opt.sel_id);
    mesh_shader.uniform("hov_color", opt.hov_color);
    mesh_shader.uniform("hov_id", opt.hov_id);
    mesh_shader.uniform("err_color", Vec3{1.0f});
    mesh_shader.uniform("err_id", 0u);

    if(opt.depth_only) GL::color_mask(false);

    if(opt.wireframe) {
        mesh_shader.uniform("color", Vec3());
        GL::enable(GL::Opt::wireframe);
        mesh.render();
        GL::disable(GL::Opt::wireframe);
    }

    mesh_shader.uniform("color", opt.color);
    mesh.render();

    if(opt.depth_only) GL::color_mask(true);
}

void Renderer::set_samples(int s) {
    samples = s;
    framebuffer.resize(window_dim, samples);
}

Scene_ID Renderer::read_id(Vec2 pos) {

    int x = (int)pos.x;
    int y = (int)(window_dim.y - pos.y - 1);

    if(id_resolve.can_read_at()) {

        GLubyte read[4] = {};
        id_resolve.read_at(0, x, y, read);
        return (int)read[0] | (int)read[1] << 8 | (int)read[2] << 16;

    } else {

        int idx = y * (int)window_dim.x * 4 + x * 4;
        assert(id_buffer && idx > 0 && idx <= window_dim.x * window_dim.y * 4);

        int a = id_buffer[idx];
        int b = id_buffer[idx + 1];
        int c = id_buffer[idx + 2];

        return a | b << 8 | c << 16;
    }
}

void Renderer::reset_depth() {
    framebuffer.clear_d();
}

void Renderer::begin_outline() {
    framebuffer.clear_d();
}

void Renderer::end_outline(const Mat4& view, BBox box) {

    Mat4 viewproj = _proj * view;
    Vec2 min, max;
    box.screen_rect(viewproj, min, max);

    Vec2 thickness = Vec2(3.0f / window_dim.x, 3.0f / window_dim.y);
    GL::Effects::outline(framebuffer, framebuffer, Gui::Color::outline, min - thickness,
                         max + thickness);
}

void Renderer::outline(const Mat4& view, Scene_Item& obj) {

    Mat4 viewproj = _proj * view;

    framebuffer.clear_d();
    obj.render(view, false, true);

    Vec2 min, max;
    obj.bbox().screen_rect(viewproj, min, max);

    Vec2 thickness = Vec2(3.0f / window_dim.x, 3.0f / window_dim.y);
    GL::Effects::outline(framebuffer, framebuffer, Gui::Color::outline, min - thickness,
                         max + thickness);
}

void Renderer::instances(Renderer::MeshOpt opt, GL::Instances& inst) {

    inst_shader.bind();
    inst_shader.uniform("use_v_id", opt.per_vert_id);
    inst_shader.uniform("use_i_id", true);
    inst_shader.uniform("id", opt.id);
    inst_shader.uniform("alpha", opt.alpha);
    inst_shader.uniform("proj", _proj);
    inst_shader.uniform("modelview", opt.modelview);
    inst_shader.uniform("solid", opt.solid_color);
    inst_shader.uniform("sel_color", opt.sel_color);
    inst_shader.uniform("sel_id", opt.sel_id);
    inst_shader.uniform("hov_color", opt.hov_color);
    inst_shader.uniform("hov_id", opt.hov_id);
    inst_shader.uniform("err_color", Vec3{1.0f});
    inst_shader.uniform("err_id", 0u);

    if(opt.depth_only) GL::color_mask(false);

    if(opt.wireframe) {
        inst_shader.uniform("color", Vec3());
        GL::enable(GL::Opt::wireframe);
        inst.render();
        GL::disable(GL::Opt::wireframe);
    }

    inst_shader.uniform("color", opt.color);
    inst.render();

    if(opt.depth_only) GL::color_mask(true);
}

void Renderer::halfedge_editor(Renderer::HalfedgeOpt opt) {

    auto [faces, spheres, cylinders, arrows] = opt.editor.shapes();

    MeshOpt fopt = MeshOpt();
    fopt.modelview = opt.modelview;
    fopt.color = opt.f_color;
    fopt.per_vert_id = true;
    fopt.sel_color = Gui::Color::outline;
    fopt.sel_id = opt.editor.select_id();
    fopt.hov_color = Gui::Color::hover;
    fopt.hov_id = opt.editor.hover_id();
    Renderer::mesh(faces, fopt);

    inst_shader.bind();
    inst_shader.uniform("use_v_id", true);
    inst_shader.uniform("use_i_id", true);
    inst_shader.uniform("solid", false);
    inst_shader.uniform("proj", _proj);
    inst_shader.uniform("modelview", opt.modelview);
    inst_shader.uniform("alpha", fopt.alpha);
    inst_shader.uniform("sel_color", Gui::Color::outline);
    inst_shader.uniform("hov_color", Gui::Color::hover);
    inst_shader.uniform("sel_id", fopt.sel_id);
    inst_shader.uniform("hov_id", fopt.hov_id);

    inst_shader.uniform("err_color", opt.err_color);
    inst_shader.uniform("err_id", opt.err_id);

    inst_shader.uniform("color", opt.v_color);
    spheres.render();
    inst_shader.uniform("color", opt.e_color);
    cylinders.render();
    inst_shader.uniform("color", opt.he_color);
    arrows.render();
}
