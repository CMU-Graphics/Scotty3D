
#include <imgui/imgui.h>
#include <nfd/nfd.h>
#include <sf_libs/stb_image_write.h>

#include "../platform/platform.h"
#include "../scene/renderer.h"

#include "manager.h"
#include "render.h"

namespace Gui {

Render::Render(Scene& scene, Vec2 dim) : ui_camera(dim), ui_render(dim) {
}

void Render::update_dim(Vec2 dim) {
    ui_camera.dim(dim);
}

bool Render::keydown(Widgets& widgets, SDL_Keysym key) {
    return false;
}

void Render::render(Scene_Maybe obj_opt, Widgets& widgets, Camera& user_cam) {

    Mat4 view = user_cam.get_view();
    Renderer& renderer = Renderer::get();

    if(!ui_camera.moving()) {

        ui_camera.render(view);

        if(render_ray_log && !ui_render.in_progress()) {
            ui_render.render_log(view);
        }

        if(visualize_bvh) {
            GL::disable(GL::Opt::depth_write);
            renderer.lines(bvh_viz, view);
            renderer.lines(bvh_active, view);
            GL::enable(GL::Opt::depth_write);
        }
    }

    if(obj_opt.has_value()) {

        Scene_Item& item = obj_opt.value();
        float scale = std::min((user_cam.pos() - item.pose().pos).norm() / 5.5f, 10.0f);

        if(item.is<Scene_Light>()) {
            Scene_Light& light = item.get<Scene_Light>();
            if(light.is_env()) return;
        }

        item.render(view);
        renderer.outline(view, item);
        widgets.render(view, item.pose().pos, scale);
    }
}

const Camera& Render::get_cam() const {
    return ui_camera.get();
}

void Render::load_cam(Vec3 pos, Vec3 center, float ar, float hfov, float ap, float dist) {

    if(ar == 0.0f) ar = ui_render.wh_ar();

    float fov = 2.0f * std::atan((1.0f / ar) * std::tan(hfov / 2.0f));
    fov = Degrees(fov);

    Camera c(Vec2{ar, 1.0f});
    c.look_at(center, pos);
    c.set_ar(ar);
    c.set_fov(fov);
    c.set_ap(ap);
    c.set_dist(dist);
    ui_camera.load(c);
}

Mode Render::UIsidebar(Manager& manager, Undo& undo, Scene& scene, Scene_Maybe obj_opt,
                       Camera& user_cam) {

    Mode mode = Mode::render;

    if(obj_opt.has_value()) {
        ImGui::Text("Object Options");
        mode = manager.item_options(undo, mode, obj_opt.value(), old_pose);
        ImGui::Separator();
    }

    ui_camera.UI(undo, user_cam);
    ImGui::Separator();

    ImGui::Text("Visualize");

    ImGui::Checkbox("Logged rays", &render_ray_log);
    ImGui::Checkbox("BVH", &visualize_bvh);

    bool update_bvh = false;

    if(visualize_bvh) {
        if(ImGui::SliderInt("Level", &bvh_level, 0, (int)bvh_levels)) {
            update_bvh = true;
        }
    }
    bvh_level = clamp(bvh_level, 0, (int)bvh_levels);

    std::string err;
    update_bvh = update_bvh || ui_render.UI(scene, ui_camera, user_cam, err);
    manager.set_error(err);

    if(update_bvh) {
        update_bvh = false;
        bvh_viz.clear();
        bvh_active.clear();
        bvh_levels = ui_render.tracer().visualize_bvh(bvh_viz, bvh_active, (size_t)bvh_level);
    }

    if(ImGui::Button("Open Render Window")) {
        ui_render.open();
    }
    return mode;
}

std::pair<float, float> Render::completion_time() const {
    return ui_render.completion_time();
}

std::string Render::headless_render(Animate& animate, Scene& scene, std::string output, bool a,
                                    int w, int h, int s, int ls, int d, float exp, bool w_from_ar) {
    if(w_from_ar) {
        w = (int)std::ceil(ui_camera.get_ar() * h);
    }
    return ui_render.headless(animate, scene, ui_camera.get(), output, a, w, h, s, ls, d, exp);
}

} // namespace Gui
