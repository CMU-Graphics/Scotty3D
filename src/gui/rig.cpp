
#include "rig.h"
#include "../scene/renderer.h"
#include "manager.h"

namespace Gui {

bool Rig::keydown(Widgets &widgets, Undo &undo, SDL_Keysym key) {

    if (!my_obj)
        return false;

#ifdef __APPLE__
    if (key.sym == SDLK_BACKSPACE && key.mod & KMOD_GUI) {
#else
    if (key.sym == SDLK_DELETE && selected) {
#endif
        undo.del_bone(my_obj->id(), selected);
        selected = nullptr;
        return true;
    }
    return false;
}

void Rig::render(Scene_Maybe obj_opt, Widgets &widgets, Camera &cam) {

    if (!obj_opt.has_value())
        return;

    Scene_Item &item = obj_opt.value();
    if (!item.is<Scene_Object>())
        return;

    Scene_Object &obj = item.get<Scene_Object>();
    if (obj.opt.shape_type != PT::Shape_Type::none)
        return;

    if (my_obj != &obj) {
        my_obj = &obj;
        selected = nullptr;
    }
    if (my_obj->rig_dirty) {
        mesh_bvh.build(obj.mesh());
        my_obj->rig_dirty = false;
    }

    Mat4 view = cam.get_view();
    obj.render(view, false, false, false, false);
    obj.armature.render(view, selected, root_selected, false);

    if (selected || root_selected) {

        widgets.active = Widget_Type::move;
        Vec3 pos;

        if (selected)
            pos = obj.armature.end_of(selected);
        else
            pos = obj.armature.base();

        float scale = std::min((cam.pos() - pos).norm() / 5.5f, 10.0f);
        widgets.render(view, pos, scale);
    }
}

void Rig::invalidate(Joint *j) {
    if (selected == j)
        selected = nullptr;
    if (new_joint == j)
        new_joint = nullptr;
}

void Rig::end_transform(Widgets &widgets, Undo &undo, Scene_Object &obj) {
    if (root_selected)
        undo.move_root(obj.id(), old_ext);
    else
        undo.move_bone(obj.id(), selected, old_ext);
    obj.set_skel_dirty();
}

void Rig::apply_transform(Widgets &widgets) {
    if (root_selected) {
        my_obj->armature.base() = widgets.apply_action(Pose::moved(old_pos)).pos;
        my_obj->set_skel_dirty();
    } else if (selected) {
        Vec3 new_pos = widgets.apply_action(Pose::moved(old_pos)).pos;
        selected->extent = new_pos - old_base;
        my_obj->set_skel_dirty();
    }
}

Vec3 Rig::selected_pos() {
    if (root_selected) {
        return my_obj->armature.base();
    } else {
        assert(selected);
        return my_obj->armature.end_of(selected);
    }
}

void Rig::select(Scene &scene, Widgets &widgets, Undo &undo, Scene_ID id, Vec3 cam, Vec2 spos,
                 Vec3 dir) {

    if (!my_obj)
        return;

    if (creating_bone) {

        undo.add_bone(my_obj->id(), new_joint);

        selected = new_joint;
        new_joint = nullptr;

        creating_bone = false;
        root_selected = false;

    } else if (widgets.want_drag()) {

        if (root_selected) {
            old_pos = my_obj->armature.base();
        } else {
            assert(selected);
            old_pos = my_obj->armature.end_of(selected);
            old_base = my_obj->armature.base_of(selected);
            old_ext = selected->extent;
        }
        widgets.start_drag(old_pos, cam, spos, dir);

    } else if (!id || id >= n_Widget_IDs) {

        selected = my_obj->armature.get_joint(id);
        root_selected = my_obj->armature.is_root_id(id);
    }
}

void Rig::clear() {
    my_obj = nullptr;
    selected = nullptr;
}

void Rig::clear_select() { selected = nullptr; }

void Rig::hover(Vec3 cam, Vec2 spos, Vec3 dir) {

    if (creating_bone) {

        assert(new_joint);
        assert(my_obj);

        Ray f(cam, dir);
        PT::Trace hit1 = mesh_bvh.hit(f);
        if (!hit1.hit)
            return;

        Ray s(hit1.position + dir * EPS_F, dir);
        PT::Trace hit2 = mesh_bvh.hit(s);

        Vec3 pos = hit1.position;
        if (hit2.hit)
            pos = 0.5f * (hit1.position + hit2.position);

        new_joint->extent = pos - old_base;
        my_obj->set_skel_dirty();
    }
}

Mode Rig::UIsidebar(Manager &manager, Undo &undo, Widgets &widgets, Scene_Maybe obj_opt) {

    if (!my_obj)
        return Mode::rig;

    if (!obj_opt.has_value())
        return Mode::rig;

    Scene_Item &item = obj_opt.value();
    if (!item.is<Scene_Object>())
        return Mode::rig;

    Scene_Object &obj = item.get<Scene_Object>();
    if (obj.opt.shape_type != PT::Shape_Type::none)
        return Mode::rig;

    if (my_obj != &obj) {
        my_obj = &obj;
        selected = nullptr;
    }
    if (my_obj->rig_dirty) {
        mesh_bvh.build(obj.mesh());
        my_obj->rig_dirty = false;
    }

    ImGui::Text("Edit Skeleton");

    if (creating_bone) {
        if (ImGui::Button("Cancel")) {
            creating_bone = false;
            my_obj->armature.erase(new_joint);
            new_joint = nullptr;
            my_obj->set_skel_dirty();
        }
    } else if (ImGui::Button("New Bone")) {

        creating_bone = true;

        if (!selected || root_selected) {
            new_joint = my_obj->armature.add_root(Vec3{0.0f});
            old_base = my_obj->armature.base();
        } else {
            new_joint = my_obj->armature.add_child(selected, Vec3{0.0f});
            old_base = my_obj->armature.end_of(selected);
        }
        my_obj->set_skel_dirty();
    }

    if (selected) {

        ImGui::Separator();
        ImGui::Text("Edit Bone");

        ImGui::DragFloat3("Extent", selected->extent.data, 0.1f);
        ImGui::DragFloat("Radius", &selected->radius, 0.05f, 0.0f,
                         std::numeric_limits<float>::max());

        ImGui::DragFloat3("Pose", selected->pose.data, 0.1f);

        if (ImGui::Button("Delete [del]")) {
            undo.del_bone(my_obj->id(), selected);
            selected = nullptr;
        }
    }

    return Mode::rig;
}

} // namespace Gui
