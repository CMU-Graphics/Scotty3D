
#pragma once

#include "../lib/mathlib.h"
#include "../rays/pathtracer.h"
#include "../scene/scene.h"

class Undo;

namespace Gui {

class Animate;

enum class Axis { X, Y, Z };

enum class Widget_Type { move, rotate, scale, bevel, count };
static const int n_Widget_Types = (int)Widget_Type::count;

enum class Widget_IDs : Scene_ID {
    none,
    x_mov,
    y_mov,
    z_mov,
    xy_mov,
    yz_mov,
    xz_mov,
    x_rot,
    y_rot,
    z_rot,
    x_scl,
    y_scl,
    z_scl,
    count
};
static const int n_Widget_IDs = (int)Widget_IDs::count;

class Widget_Camera {
public:
    Widget_Camera(Vec2 screen_dim)
        : screen_dim(screen_dim), render_cam(screen_dim), saved_cam(screen_dim) {
        generate_cage();
    }

    bool UI(Undo& undo, Camera& user_cam);
    void render(const Mat4& view);

    void load(Camera c);
    const Camera& get() const {
        return render_cam;
    }
    void ar(Camera& user_cam, float _ar);
    float get_ar() const {
        return cam_ar;
    }
    bool moving() const {
        return moving_camera;
    }
    void dim(Vec2 d) {
        screen_dim = d;
    }

private:
    float cam_fov = 90.0f, cam_ar = 1.7778f, cam_ap = 0.0f, cam_dist = 1.0f;
    bool moving_camera = false;
    Vec2 screen_dim;
    Camera render_cam, saved_cam;
    GL::Lines cam_cage;

    Camera old = render_cam;
    float old_ar, old_fov, old_ap, old_dist;

    void update_cameras(Camera& user_cam);
    void generate_cage();
};

class Widget_Render {
public:
    Widget_Render(Vec2 dim);
    void open();

    bool UI(Scene& scene, Widget_Camera& cam, Camera& user_cam, std::string& err);

    void animate(Scene& scene, Widget_Camera& cam, Camera& user_cam, int max_frame);
    std::string step(Animate& animate, Scene& scene);

    std::string headless(Animate& animate, Scene& scene, const Camera& cam, std::string output,
                         bool a, int w, int h, int s, int ls, int d, float exp);

    void log_ray(const Ray& ray, float t, Spectrum color = Spectrum{1.0f});
    void render_log(const Mat4& view) const;

    PT::Pathtracer& tracer() {
        return pathtracer;
    }
    bool rendered() const {
        return has_rendered;
    }
    std::pair<float, float> completion_time() const {
        return pathtracer.completion_time();
    }
    bool in_progress() const {
        return pathtracer.in_progress();
    }
    float wh_ar() const {
        return (float)out_w / (float)out_h;
    }

private:
    void begin(Scene& scene, Widget_Camera& cam, Camera& user_cam);

    mutable std::mutex log_mut;
    GL::Lines ray_log;

    int out_w, out_h, out_samples = 32, out_area_samples = 8, out_depth = 4;
    float exposure = 1.0f;

    bool has_rendered = false;
    bool render_window = false, render_window_focus = false;

    int method = 1;
    bool animating = false, init = false;
    int next_frame = 0, max_frame = 0;

    char output_path[256] = {};
    std::string folder;

    GL::MSAA msaa;
    PT::Pathtracer pathtracer;
};

class Widgets {
public:
    Widgets();
    Widget_Type active = Widget_Type::move;

    void end_drag();
    Pose apply_action(const Pose& pose);
    void start_drag(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir);
    void drag_to(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir, bool scale_invert);

    void select(Scene_ID id);
    void render(const Mat4& view, Vec3 pos, float scl);
    bool action_button(Widget_Type act, std::string name, bool wrap = true);

    bool want_drag();
    bool is_dragging();

private:
    void generate_lines(Vec3 pos);
    bool to_axis(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3& hit);
    bool to_plane(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3 norm, Vec3& hit);

    // interface data
    Axis axis = Axis::X;
    Vec3 drag_start, drag_end;
    Vec2 bevel_start, bevel_end;
    bool dragging = false, drag_plane = false;
    bool start_dragging = false;

    // render data
    GL::Lines lines;
    Scene_Object x_mov, y_mov, z_mov;
    Scene_Object xy_mov, yz_mov, xz_mov;
    Scene_Object x_rot, y_rot, z_rot;
    Scene_Object x_scl, z_scl, y_scl;
};

} // namespace Gui
