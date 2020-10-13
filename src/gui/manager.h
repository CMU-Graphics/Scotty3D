
#pragma once

#include <SDL2/SDL.h>
#include <imgui/imgui.h>

#ifndef SCOTTY3D_BUILD_REF
#include "../student/debug.h"
#endif

#include "../lib/mathlib.h"
#include "../util/camera.h"

#include "../scene/scene.h"
#include "../scene/undo.h"

#include "animate.h"
#include "layout.h"
#include "model.h"
#include "render.h"
#include "rig.h"
#include "widgets.h"

namespace Gui {

struct Settings {
    int samples = 4;
};

enum class Mode { layout, model, render, rig, animate, simulate };

#define RGBv(n, r, g, b) static inline const Vec3 n = Vec3(r##.0f, g##.0f, b##.0f) / 255.0f;
struct Color {
    RGBv(black, 0, 0, 0);
    RGBv(outline, 242, 153, 41);
    RGBv(hover, 102, 102, 204);
    RGBv(baseplane, 71, 71, 71);
    RGBv(background, 58, 58, 58);
    RGBv(red, 163, 66, 81);
    RGBv(green, 124, 172, 40);
    RGBv(blue, 64, 127, 193);
    static Vec3 axis(Axis a);
};
#undef RGBv

class Manager {
public:
    Manager(Scene &scene, Vec2 window_dim);

    // Input
    void update_dim(Vec2 dim);
    bool keydown(Undo &undo, SDL_Keysym key, Scene &scene, Camera &cam);

    Rig &get_rig();
    Render &get_render();
    Animate &get_animate();
    const Settings &get_settings() const;
    void set_file(std::string save);

    // Object interaction
    bool select(Scene &scene, Undo &undo, Scene_ID id, Vec3 cam, Vec2 spos, Vec3 dir);
    void clear_select();
    void set_select(Scene_ID id);

    void hover(Vec2 pixel, Vec3 cam, Vec2 spos, Vec3 dir);
    void drag_to(Scene &scene, Vec3 cam, Vec2 spos, Vec3 dir);
    void end_drag(Undo &undo, Scene &scene);

    void render_3d(Scene &scene, Camera &camera);
    void render_ui(Scene &scene, Undo &undo, Camera &camera);

    void set_error(std::string msg);
    void invalidate_obj(Scene_ID id);
    void light_edit_gui(Undo &undo, Scene_Light &light);
    void material_edit_gui(Undo &undo, Scene_ID id, Material &material);
    Mode item_options(Undo &undo, Mode cur_mode, Scene_Item &item, Pose &old_pose);

    static bool wrap_button(std::string label);

private:
    void UIerror();
    void UIstudent();
    void UIsettings();
    void UInew_obj(Undo &undo);
    void UInew_light(Scene &scene, Undo &undo);
    float UImenu(Scene &scene, Undo &undo);
    void UIsidebar(Scene &scene, Undo &undo, float menu_height, Camera &cam);
    void load_image(Scene_Light &image);
    void frame(Scene &scene, Camera &cam);

    static inline const char *scene_file_types = "dae,obj,fbx,glb,gltf,3ds,blend,stl,ply";
    static inline const char *image_file_types = "exr,hdr,hdri,jpg,jpeg,png,tga,bmp,psd,gif";

    void render_selected(Scene_Object &obj);
    void load_scene(Scene &scene, Undo &undo, bool clear);
    void write_scene(Scene &scene);
    void save_scene(Scene &scene);

    Mode mode = Mode::layout;
    Layout layout;
    Model model;
    Render render;
    Rig rig;
    Animate animate;

    Settings settings;
    bool error_shown = false;
    bool debug_shown = false;
    bool settings_shown = false;
    bool new_obj_window = false, new_obj_focus = false;
    bool new_light_window = false, new_light_focus = false;
    std::string error_msg, save_file;

    Widgets widgets;
    GL::Lines baseplane;
    void create_baseplane();
    Vec2 window_dim;
};

}; // namespace Gui
