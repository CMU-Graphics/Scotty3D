
#pragma once

#include "../rays/tri_mesh.h"
#include "widgets.h"
#include <SDL2/SDL.h>

namespace Gui {

enum class Mode;
class Manager;

class Rig {
public:
    bool keydown(Widgets& widgets, Undo& undo, SDL_Keysym key);

    void select(Scene& scene, Widgets& widgets, Undo& undo, Scene_ID id, Vec3 cam, Vec2 spos,
                Vec3 dir);
    void hover(Vec3 cam, Vec2 spos, Vec3 dir);

    void end_transform(Widgets& widgets, Undo& undo, Scene_Object& obj);
    void apply_transform(Widgets& widgets);

    Vec3 selected_pos();
    void clear_select();
    void clear();
    void invalidate(Joint* j);
    void invalidate(Skeleton::IK_Handle* j);

    void render(Scene_Maybe obj_opt, Widgets& widgets, Camera& cam);
    Mode UIsidebar(Manager& manager, Undo& undo, Widgets& widgets, Scene_Maybe obj);

private:
    bool creating_bone = false;
    bool root_selected = false;
    Vec3 old_pos, old_base, old_ext;
    float old_r;

    Scene_Object* my_obj = nullptr;
    Joint* selected = nullptr;
    Skeleton::IK_Handle* handle = nullptr;
    Joint* new_joint = nullptr;
    PT::Tri_Mesh mesh_bvh;
};

} // namespace Gui
