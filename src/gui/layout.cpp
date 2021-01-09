
#include <imgui/imgui.h>

#include "layout.h"
#include "manager.h"

#include "../scene/renderer.h"
#include "../scene/undo.h"

namespace Gui {

bool Layout::keydown(Widgets& widgets, SDL_Keysym key) {
    return false;
}

void Layout::render(Scene_Maybe obj_opt, Widgets& widgets, Camera& cam) {

    if(!obj_opt.has_value()) return;
    Scene_Item& item = obj_opt.value();

    if(item.is<Scene_Light>()) {
        Scene_Light& light = item.get<Scene_Light>();
        if(light.is_env()) return;
    }

    Pose& pose = item.pose();
    float scale = std::min((cam.pos() - pose.pos).norm() / 5.5f, 10.0f);
    Mat4 view = cam.get_view();

    item.render(view);
    Renderer::get().outline(view, item);
    widgets.render(view, pose.pos, scale);
}

Scene_ID Layout::selected() const {
    return selected_mesh;
}

void Layout::clear_select() {
    selected_mesh = 0;
}

void Layout::set_selected(Scene_ID id) {
    selected_mesh = id;
}

Vec3 Layout::selected_pos(Scene& scene) {

    auto obj = scene.get(selected_mesh);
    if(obj.has_value()) return obj->get().pose().pos;
    return {};
}

void Layout::select(Scene& scene, Widgets& widgets, Scene_ID id, Vec3 cam, Vec2 spos, Vec3 dir) {

    if(widgets.want_drag()) {

        Scene_Item& item = scene.get(selected_mesh).value();
        Pose& pose = item.pose();
        widgets.start_drag(pose.pos, cam, spos, dir);
        old_pose = pose;

    } else if(id >= n_Widget_IDs) {
        selected_mesh = id;
    }
}

void Layout::end_transform(Undo& undo, Scene_Item& obj) {
    undo.update_pose(obj.id(), old_pose);
    old_pose = {};
}

Mode Layout::UIsidebar(Manager& manager, Undo& undo, Widgets& widgets, Scene_Maybe obj_opt) {

    if(!obj_opt.has_value()) return Mode::layout;
    ImGui::Text("Object Options");
    Mode ret = manager.item_options(undo, Mode::layout, obj_opt.value(), old_pose);
    ImGui::Separator();
    return ret;
}

void Layout::apply_transform(Scene_Item& obj, Widgets& widgets) {
    obj.pose() = widgets.apply_action(old_pose);
}

} // namespace Gui
