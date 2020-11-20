
#include "undo.h"
#include "../gui/animate.h"
#include "../gui/manager.h"
#include "../gui/rig.h"

Undo::Undo(Scene &sc, Gui::Manager &man) : scene(sc), gui(man) {}

void Undo::reset() {
    undos = {};
    redos = {};
}

template <typename R, typename U> class Action : public Action_Base {
public:
    Action(R &&r, U &&u)
        : _undo(std::forward<decltype(u)>(u)), _redo(std::forward<decltype(r)>(r)){};
    ~Action() {}

private:
    U _undo;
    R _redo;
    void undo() { _undo(); }
    void redo() { _redo(); }
};

template <typename R, typename U> void Undo::action(R &&redo, U &&undo) {
    action(std::make_unique<Action<R, U>>(std::move(redo), std::move(undo)));
}

void Undo::update_mesh_full(Scene_ID id, Halfedge_Mesh &&old_mesh) {

    Scene_Object &obj = scene.get_obj(id);
    Halfedge_Mesh new_mesh;
    obj.copy_mesh(new_mesh);

    action(
        [id, this, nm = std::move(new_mesh)]() mutable {
            Scene_Object &obj = scene.get_obj(id);
            obj.set_mesh(nm);
        },
        [id, this, om = std::move(old_mesh)]() mutable {
            Scene_Object &obj = scene.get_obj(id);
            obj.set_mesh(om);
        });
}

void Undo::move_root(Scene_ID id, Vec3 old) {

    Scene_Object &obj = scene.get_obj(id);
    Vec3 np = obj.armature.base();

    action(
        [this, id, np]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.base() = np;
            obj.set_skel_dirty();
        },
        [this, id, op = old]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.base() = op;
            obj.set_skel_dirty();
        });
}

void Undo::del_bone(Scene_ID id, Joint *j) {

    Scene_Object &obj = scene.get_obj(id);
    obj.armature.erase(j);
    obj.set_skel_dirty();

    action(
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.erase(j);
            gui.get_rig().invalidate(j);
            gui.get_animate().invalidate(j);
            obj.set_skel_dirty();
        },
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.restore(j);
            obj.set_skel_dirty();
        });
}

void Undo::del_handle(Scene_ID id, Skeleton::IK_Handle *j) {

    Scene_Object &obj = scene.get_obj(id);
    obj.armature.erase(j);
    obj.set_skel_dirty();

    action(
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.erase(j);
            gui.get_animate().invalidate(j);
            gui.get_rig().invalidate(j);
            obj.set_skel_dirty();
        },
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.restore(j);
            obj.set_skel_dirty();
        });
}

void Undo::move_bone(Scene_ID id, Joint *j, Vec3 old) {
    action(
        [this, id, j, ne = j->extent]() {
            Scene_Object &obj = scene.get_obj(id);
            j->extent = ne;
            obj.set_skel_dirty();
        },
        [this, id, j, oe = old]() {
            Scene_Object &obj = scene.get_obj(id);
            j->extent = oe;
            obj.set_skel_dirty();
        });
}

void Undo::move_handle(Scene_ID id, Skeleton::IK_Handle *j, Vec3 old) {
    action(
        [this, id, j, ne = j->target]() {
            Scene_Object &obj = scene.get_obj(id);
            j->target = ne;
            obj.set_skel_dirty();
        },
        [this, id, j, oe = old]() {
            Scene_Object &obj = scene.get_obj(id);
            j->target = oe;
            obj.set_skel_dirty();
        });
}

void Undo::pose_bone(Scene_ID id, Joint *j, Vec3 old) {
    action(
        [this, id, j, ne = j->pose]() {
            Scene_Object &obj = scene.get_obj(id);
            j->pose = ne;
            obj.set_pose_dirty();
        },
        [this, id, j, oe = old]() {
            Scene_Object &obj = scene.get_obj(id);
            j->pose = oe;
            obj.set_pose_dirty();
        });
}

void Undo::add_handle(Scene_ID id, Skeleton::IK_Handle *j) {
    action(
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.restore(j);
            obj.set_skel_dirty();
        },
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.erase(j);
            gui.get_rig().invalidate(j);
            gui.get_animate().invalidate(j);
            obj.set_skel_dirty();
        });
}

void Undo::add_bone(Scene_ID id, Joint *j) {

    action(
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.restore(j);
            obj.set_skel_dirty();
        },
        [this, id, j]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.armature.erase(j);
            gui.get_rig().invalidate(j);
            gui.get_animate().invalidate(j);
            obj.set_skel_dirty();
        });
}

void Undo::del_obj(Scene_ID id) {
    scene.erase(id);
    gui.invalidate_obj(id);
    action(
        [id, this]() {
            scene.erase(id);
            gui.invalidate_obj(id);
        },
        [id, this]() { scene.restore(id); });
}

Scene_Object &Undo::add_obj(GL::Mesh &&mesh, std::string name) {
    Scene_ID id = scene.add({}, std::move(mesh), name);
    scene.restore(id);
    action([id, this]() { scene.restore(id); },
           [id, this]() {
               scene.erase(id);
               gui.invalidate_obj(id);
           });
    return scene.get_obj(id);
}

Scene_Object &Undo::add_obj(Halfedge_Mesh &&mesh, std::string name) {
    Scene_ID id = scene.add({}, std::move(mesh), name);
    scene.restore(id);
    action([id, this]() { scene.restore(id); },
           [id, this]() {
               scene.erase(id);
               gui.invalidate_obj(id);
           });
    return scene.get_obj(id);
}

void Undo::add_light(Scene_Light &&light) {
    Scene_ID id = scene.add(std::move(light));
    scene.restore(id);
    action([id, this]() { scene.restore(id); },
           [id, this]() {
               scene.erase(id);
               gui.invalidate_obj(id);
           });
}

void Undo::update_light(Scene_ID id, Scene_Light::Options old) {

    Scene_Light &light = scene.get_light(id);

    action(
        [id, this, no = light.opt]() {
            Scene_Light &light = scene.get_light(id);
            light.opt = no;
            light.dirty();
        },
        [id, this, oo = old]() {
            Scene_Light &light = scene.get_light(id);
            light.opt = oo;
            light.dirty();
        });
}

void Undo::update_material(Scene_ID id, Material::Options old) {

    Scene_Object &obj = scene.get_obj(id);

    action(
        [id, this, nm = obj.material.opt]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.material.opt = nm;
        },
        [id, this, om = old]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.material.opt = om;
        });
}

void Undo::update_object(Scene_ID id, Scene_Object::Options old) {

    Scene_Object &obj = scene.get_obj(id);

    if (obj.opt.shape_type != PT::Shape_Type::none && old.shape_type == PT::Shape_Type::none) {

        Halfedge_Mesh old_mesh;
        obj.copy_mesh(old_mesh);

        action(
            [id, this, no = obj.opt]() {
                Scene_Object &obj = scene.get_obj(id);
                obj.opt = no;
                obj.set_mesh_dirty();
            },
            [id, this, oo = old, om = std::move(old_mesh)]() mutable {
                Scene_Object &obj = scene.get_obj(id);
                obj.opt = oo;
                obj.set_mesh(om);
            });

        return;
    }

    if (obj.opt.shape_type == PT::Shape_Type::none && old.shape_type != PT::Shape_Type::none) {

        action(
            [id, this, no = obj.opt, ot = old.shape_type]() {
                Scene_Object &obj = scene.get_obj(id);
                obj.opt = no;
                obj.try_make_editable(ot);
            },
            [id, this, oo = old]() {
                Scene_Object &obj = scene.get_obj(id);
                obj.opt = oo;
                obj.set_mesh_dirty();
            });

        return;
    }

    action(
        [id, this, no = obj.opt]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.opt = no;
            obj.set_mesh_dirty();
        },
        [id, this, oo = old]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.opt = oo;
            obj.set_mesh_dirty();
        });
}

void Undo::update_pose(Scene_ID id, Pose old) {

    Scene_Item &item = *scene.get(id);

    action(
        [id, this, np = item.pose()]() {
            Scene_Item &item = *scene.get(id);
            item.pose() = np;
        },
        [id, this, op = old]() {
            Scene_Item &item = *scene.get(id);
            item.pose() = op;
        });
}

void Undo::update_camera(Gui::Widget_Camera &widget, Camera old) {

    Camera newc = widget.get();
    action(
        [&widget, nc = newc]() { widget.load(nc.center(), nc.pos(), nc.get_ar(), nc.get_fov()); },
        [&widget, oc = old]() { widget.load(oc.center(), oc.pos(), oc.get_ar(), oc.get_fov()); });
}

void Undo::anim_pose_bones(Scene_ID id, float t) {

    Scene_Object &obj = scene.get_obj(id);

    bool had = obj.anim.splines.has(t) || obj.armature.has_keyframes();

    Pose old_pose = obj.anim.at(t);
    Pose new_pose = obj.pose;
    auto old_joints = obj.armature.at(t);
    auto new_joints = obj.armature.now();

    obj.anim.set(t, new_pose);
    obj.armature.set(t);

    action(
        [=]() {
            Scene_Object &obj = scene.get_obj(id);
            obj.anim.set(t, new_pose);
            obj.armature.set(t, new_joints);
            gui.get_animate().refresh(scene);
        },
        [=]() {
            Scene_Object &obj = scene.get_obj(id);
            if (had) {
                obj.anim.set(t, old_pose);
                obj.armature.set(t, old_joints);
            } else {
                obj.anim.splines.erase(t);
                obj.armature.erase(t);
            }
        });
}

void Undo::anim_pose(Scene_ID id, float t) {

    Scene_Item &item = *scene.get(id);

    bool had = item.animation().splines.has(t);
    Pose old = item.animation().at(t);
    Pose p = item.pose();

    item.animation().set(t, p);

    action(
        [=]() {
            Scene_Item &item = *scene.get(id);
            item.animation().set(t, p);
            gui.get_animate().refresh(scene);
        },
        [=]() {
            Scene_Item &item = *scene.get(id);
            if (had)
                item.animation().set(t, old);
            else
                item.animation().splines.erase(t);
        });
}

void Undo::anim_camera(Gui::Anim_Camera &anim, float t, const Camera &cam) {

    bool had = anim.splines.has(t);
    Camera oldc = anim.at(t);
    Camera newc = cam;

    anim.set(t, newc);

    action(
        [=, &anim]() {
            anim.set(t, newc);
            gui.get_animate().refresh(scene);
        },
        [=, &anim]() {
            if (had)
                anim.set(t, oldc);
            else
                anim.splines.erase(t);
        });
}

void Undo::anim_light(Scene_ID id, float t) {

    Scene_Light &item = scene.get_light(id);

    bool had_l = item.lanim.splines.has(t);
    bool had_p = item.anim.splines.has(t);

    Pose old_pose = item.anim.at(t);
    Pose new_pose = item.pose;

    Scene_Light::Options old_l;
    item.lanim.at(t, old_l);
    Scene_Light::Options new_l = item.opt;

    item.anim.set(t, new_pose);
    item.lanim.set(t, new_l);

    action(
        [=]() {
            Scene_Light &item = scene.get_light(id);
            item.lanim.set(t, new_l);
            item.anim.set(t, new_pose);
            gui.get_animate().refresh(scene);
        },
        [=]() {
            Scene_Light &item = scene.get_light(id);
            if (had_l)
                item.lanim.set(t, old_l);
            else
                item.lanim.splines.erase(t);
            if (had_p)
                item.anim.set(t, old_pose);
            else
                item.anim.splines.erase(t);
            item.dirty();
        });
}

void Undo::action(std::unique_ptr<Action_Base> &&action) {
    redos = {};
    undos.push(std::move(action));
}

void Undo::undo() {
    if (undos.empty())
        return;
    undos.top()->undo();
    redos.push(std::move(undos.top()));
    undos.pop();
}

void Undo::redo() {
    if (redos.empty())
        return;
    redos.top()->redo();
    undos.push(std::move(redos.top()));
    redos.pop();
}
