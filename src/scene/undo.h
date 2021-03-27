
#pragma once

#include <memory>
#include <stack>

#include "../gui/widgets.h"
#include "scene.h"

namespace Gui {
class Manager;
class Anim_Camera;
class Rig;
} // namespace Gui

class Action_Base {
    virtual void undo() = 0;
    virtual void redo() = 0;
    friend class Undo;
    friend class Action_Bundle;

public:
    virtual ~Action_Base() = default;
};

class Action_Bundle : public Action_Base {
    void undo() {
        for(auto& a : list) a->undo();
    }
    void redo() {
        for(auto i = list.rbegin(); i != list.rend(); i++) (*i)->redo();
    }

    std::vector<std::unique_ptr<Action_Base>> list;

public:
    Action_Bundle(std::vector<std::unique_ptr<Action_Base>>&& bundle) : list(std::move(bundle)){};
    ~Action_Bundle() = default;
};

template<typename T> class MeshOp : public Action_Base {
    void undo() {
        Scene_Object& obj = scene.get_obj(id);
        obj.set_mesh(mesh);
    }
    void redo() {
        Scene_Object& obj = scene.get_obj(id);
        auto sel = obj.set_mesh(mesh, eid);
        op(obj.get_mesh(), sel);
        obj.get_mesh().do_erase();
    }
    Scene& scene;
    Scene_ID id;
    unsigned int eid;
    T op;
    Halfedge_Mesh mesh;

public:
    MeshOp(Scene& s, Scene_ID i, unsigned int e, Halfedge_Mesh&& m, T&& t)
        : scene(s), id(i), eid(e), op(t), mesh(std::move(m)) {
    }
    ~MeshOp() = default;
};

class Undo {
public:
    Undo(Scene& scene, Gui::Manager& man);

    void add_light(Scene_Light&& mesh);
    void add_particles(Scene_Particles&& particles);
    Scene_Object& add_obj(GL::Mesh&& mesh, std::string name);
    Scene_Object& add_obj(Halfedge_Mesh&& mesh, std::string name);

    void del_obj(Scene_ID id);
    void update_pose(Scene_ID id, Pose old);

    void del_bone(Scene_ID id, Joint* j);
    void add_bone(Scene_ID id, Joint* j);
    void move_root(Scene_ID id, Vec3 old);
    void move_bone(Scene_ID id, Joint* j, Vec3 old);
    void pose_bone(Scene_ID id, Joint* j, Vec3 old);
    void rad_bone(Scene_ID id, Joint* j, float old);

    void del_handle(Scene_ID id, Skeleton::IK_Handle* j);
    void add_handle(Scene_ID id, Skeleton::IK_Handle* j);
    void move_handle(Scene_ID id, Skeleton::IK_Handle* j, Vec3 old);

    void update_material(Scene_ID id, Material::Options old);
    void update_light(Scene_ID id, Scene_Light::Options old);
    void update_object(Scene_ID id, Scene_Object::Options old);
    void update_camera(Gui::Widget_Camera& widget, Camera old);
    void update_particles(Scene_ID id, Scene_Particles::Options old);

    template<typename T>
    void update_mesh(Scene_ID id, Halfedge_Mesh&& old, unsigned int e_id, T&& op) {
        std::stack<std::unique_ptr<Action_Base>> empty;
        redos.swap(empty);
        undos.push(std::make_unique<MeshOp<T>>(scene, id, e_id, std::move(old), std::move(op)));
        total_actions++;
    }
    void update_mesh_full(Scene_ID id, Halfedge_Mesh&& old_mesh);

    void anim_clear_light(Scene_ID id, float t);
    void anim_clear_object(Scene_ID id, float t);
    void anim_clear_particles(Scene_ID id, float t);
    void anim_clear_camera(Gui::Anim_Camera& anim, float t);

    void anim_light(Scene_ID id, float t);
    void anim_particles(Scene_ID id, float t);
    void anim_object(Scene_ID id, float t);
    void anim_camera(Gui::Anim_Camera& anim, float t, const Camera& cam);

    void anim_crop(Scene_ID id, float t);
    void anim_crop_camera(Gui::Anim_Camera& anim, float t);

    void set_max_frame(Gui::Animate& anim, unsigned int fnew, unsigned int fold);

    void undo();
    void redo();
    void reset();
    size_t n_actions();
    void inc_actions();
    void bundle_last(size_t n);

private:
    Scene& scene;
    Gui::Manager& gui;

    template<typename R, typename U> void action(R&& redo, U&& undo);
    void action(std::unique_ptr<Action_Base>&& action);

    std::stack<std::unique_ptr<Action_Base>> undos;
    std::stack<std::unique_ptr<Action_Base>> redos;
    size_t total_actions = 0;
};
