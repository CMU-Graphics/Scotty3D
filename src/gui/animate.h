
#pragma once

#include "widgets.h"
#include <SDL2/SDL.h>
#include <set>

namespace Gui {

enum class Mode;
class Manager;
class Simulate;

class Anim_Camera {
public:
    Anim_Camera(Vec2 dim) : dim(dim) {
    }

    Camera at(float t) const;
    void set(float t, const Camera& cam);

    Splines<Vec3, Quat, float, float, float, float> splines;

private:
    Vec2 dim;
};

class Animate {
public:
    Animate(Simulate& sim, Vec2 screen_dim)
        : ui_camera(screen_dim), anim_camera(screen_dim), ui_render(screen_dim), simulate(sim) {
    }

    void update_dim(Vec2 dim);
    bool keydown(Widgets& widgets, Undo& undo, Scene_ID sel, SDL_Keysym key);

    Vec3 selected_pos(Scene_Item& item);
    void end_transform(Undo& undo, Scene_Item& obj);
    void apply_transform(Widgets& widgets, Scene_Item& obj);
    bool select(Scene& scene, Widgets& widgets, Scene_ID selected, Scene_ID id, Vec3 cam, Vec2 spos,
                Vec3 dir);

    void render(Scene& scene, Scene_Maybe obj_opt, Widgets& widgets, Camera& cam);
    void timeline(Manager& manager, Undo& undo, Scene& scene, Scene_Maybe obj, Camera& user_cam);
    void UIsidebar(Manager& manager, Undo& undo, Scene_Maybe obj_opt, Camera& user_cam);

    void clear();
    void update(Scene& scene);
    void refresh(Scene& scene);
    void load_cam(Vec3 pos, Vec3 front, float ar, float fov, float ap, float dist);
    void step_sim(Scene& scene);

    std::string pump_output(Scene& scene);
    Camera set_time(Scene& scene, float time);
    float fps() const;
    int n_frames() const;
    const Anim_Camera& camera() const;
    Anim_Camera& camera();
    Camera current_camera() const;
    void set(int n_frames, int fps, bool replace);
    void set_max(int frames);
    void invalidate(Skeleton::IK_Handle* handle);
    void invalidate(Joint* handle);

private:
    Uint64 last_frame = 0;
    bool playing = false;
    int frame_rate = 24;
    int max_frame = 96;
    int current_frame = 0;
    int displayed_frame = 0;

    Widget_Camera ui_camera;
    Widget_Render ui_render;
    Anim_Camera anim_camera;
    Simulate& simulate;

    Joint* joint_select = nullptr;
    Skeleton::IK_Handle* handle_select = nullptr;
    unsigned int joint_id_offset = 0;

    Pose old_pose;
    Mat4 old_T;
    Vec3 old_euler, old_pos;

    bool visualize_splines = false;
    bool camera_selected = false;
    Scene_ID prev_selected = 0;
    std::unordered_map<Scene_ID, GL::Lines> spline_cache;
    void make_spline(Scene_ID id, const Anim_Pose& pose);
    void camera_spline();
};

} // namespace Gui
