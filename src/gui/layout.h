
#pragma once

#include <SDL2/SDL.h>

#include "../scene/scene.h"
#include "../util/camera.h"

#include "widgets.h"

namespace Gui {

enum class Mode;
class Manager;

class Layout {
public:
    bool keydown(Widgets& widgets, SDL_Keysym key);
    void render(Scene_Maybe obj_opt, Widgets& widgets, Camera& cam);
    void select(Scene& scene, Widgets& widgets, Scene_ID id, Vec3 cam, Vec2 spos, Vec3 dir);
    Vec3 selected_pos(Scene& scene);

    void apply_transform(Scene_Item& obj, Widgets& widgets);
    void end_transform(Undo& undo, Scene_Item& obj);
    Mode UIsidebar(Manager& manager, Undo& undo, Widgets& widgets, Scene_Maybe obj);

    Scene_ID selected() const;
    void clear_select();
    void set_selected(Scene_ID id);

private:
    Pose old_pose;
    Scene_ID selected_mesh = (Scene_ID)Widget_IDs::none;
};

} // namespace Gui
