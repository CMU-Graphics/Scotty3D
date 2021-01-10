
#include <imgui/imgui.h>
#include <iomanip>
#include <iostream>
#include <nfd/nfd.h>
#include <sf_libs/stb_image_write.h>
#include <sstream>

#include "animate.h"
#include "manager.h"
#include "widgets.h"

#include "../geometry/util.h"
#include "../platform/platform.h"
#include "../scene/renderer.h"

namespace Gui {

Widgets::Widgets() : lines(1.0f) {

    x_mov = Scene_Object((Scene_ID)Widget_IDs::x_mov, Pose::rotated(Vec3{0.0f, 0.0f, -90.0f}),
                         Util::arrow_mesh(0.03f, 0.075f, 1.0f));
    y_mov = Scene_Object((Scene_ID)Widget_IDs::y_mov, {}, Util::arrow_mesh(0.03f, 0.075f, 1.0f));
    z_mov = Scene_Object((Scene_ID)Widget_IDs::z_mov, Pose::rotated(Vec3{90.0f, 0.0f, 0.0f}),
                         Util::arrow_mesh(0.03f, 0.075f, 1.0f));

    xy_mov = Scene_Object((Scene_ID)Widget_IDs::xy_mov, Pose::rotated(Vec3{-90.0f, 0.0f, 0.0f}),
                          Util::square_mesh(0.1f));
    yz_mov = Scene_Object((Scene_ID)Widget_IDs::yz_mov, Pose::rotated(Vec3{0.0f, 0.0f, -90.0f}),
                          Util::square_mesh(0.1f));
    xz_mov = Scene_Object((Scene_ID)Widget_IDs::xz_mov, {}, Util::square_mesh(0.1f));

    x_rot = Scene_Object((Scene_ID)Widget_IDs::x_rot, Pose::rotated(Vec3{0.0f, 0.0f, -90.0f}),
                         Util::torus_mesh(0.975f, 1.0f));
    y_rot = Scene_Object((Scene_ID)Widget_IDs::y_rot, {}, Util::torus_mesh(0.975f, 1.0f));
    z_rot = Scene_Object((Scene_ID)Widget_IDs::z_rot, Pose::rotated(Vec3{90.0f, 0.0f, 0.0f}),
                         Util::torus_mesh(0.975f, 1.0f));

    x_scl = Scene_Object((Scene_ID)Widget_IDs::x_scl, Pose::rotated(Vec3{0.0f, 0.0f, -90.0f}),
                         Util::scale_mesh());
    y_scl = Scene_Object((Scene_ID)Widget_IDs::y_scl, {}, Util::scale_mesh());
    z_scl = Scene_Object((Scene_ID)Widget_IDs::z_scl, Pose::rotated(Vec3{90.0f, 0.0f, 0.0f}),
                         Util::scale_mesh());

#define setcolor(o, c) o.material.opt.albedo = Spectrum((c).x, (c).y, (c).z);
    setcolor(x_mov, Color::red);
    setcolor(y_mov, Color::green);
    setcolor(z_mov, Color::blue);
    setcolor(xy_mov, Color::blue);
    setcolor(yz_mov, Color::red);
    setcolor(xz_mov, Color::green);
    setcolor(x_rot, Color::red);
    setcolor(y_rot, Color::green);
    setcolor(z_rot, Color::blue);
    setcolor(x_scl, Color::red);
    setcolor(y_scl, Color::green);
    setcolor(z_scl, Color::blue);
#undef setcolor
}

void Widgets::generate_lines(Vec3 pos) {
    auto add_axis = [&](int axis) {
        Vec3 start = pos;
        start[axis] -= 10000.0f;
        Vec3 end = pos;
        end[axis] += 10000.0f;
        Vec3 color = Color::axis((Axis)axis);
        lines.add(start, end, color);
    };
    if(drag_plane) {
        add_axis(((int)axis + 1) % 3);
        add_axis(((int)axis + 2) % 3);
    } else {
        add_axis((int)axis);
    }
}

bool Widgets::action_button(Widget_Type act, std::string name, bool wrap) {
    bool is_active = act == active;
    if(is_active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
    bool clicked = wrap ? Manager::wrap_button(name) : ImGui::Button(name.c_str());
    if(is_active) ImGui::PopStyleColor();
    if(clicked) active = act;
    return clicked;
};

void Widgets::render(const Mat4& view, Vec3 pos, float scl) {

    Renderer& r = Renderer::get();
    r.reset_depth();

    Vec3 scale(scl);
    r.lines(lines, view, Mat4::I, 0.5f);

    if(dragging && (active == Widget_Type::move || active == Widget_Type::scale)) return;

    if(active == Widget_Type::move) {

        x_mov.pose.scale = scale;
        x_mov.pose.pos = pos + Vec3(0.15f * scl, 0.0f, 0.0f);
        x_mov.render(view, true);

        y_mov.pose.scale = scale;
        y_mov.pose.pos = pos + Vec3(0.0f, 0.15f * scl, 0.0f);
        y_mov.render(view, true);

        z_mov.pose.scale = scale;
        z_mov.pose.pos = pos + Vec3(0.0f, 0.0f, 0.15f * scl);
        z_mov.render(view, true);

        xy_mov.pose.scale = scale;
        xy_mov.pose.pos = pos + Vec3(0.45f * scl, 0.45f * scl, 0.0f);
        xy_mov.render(view, true);

        yz_mov.pose.scale = scale;
        yz_mov.pose.pos = pos + Vec3(0.0f, 0.45f * scl, 0.45f * scl);
        yz_mov.render(view, true);

        xz_mov.pose.scale = scale;
        xz_mov.pose.pos = pos + Vec3(0.45f * scl, 0.0f, 0.45f * scl);
        xz_mov.render(view, true);

    } else if(active == Widget_Type::rotate) {

        if(!dragging || axis == Axis::X) {
            x_rot.pose.scale = scale;
            x_rot.pose.pos = pos;
            x_rot.render(view, true);
        }
        if(!dragging || axis == Axis::Y) {
            y_rot.pose.scale = scale;
            y_rot.pose.pos = pos;
            y_rot.render(view, true);
        }
        if(!dragging || axis == Axis::Z) {
            z_rot.pose.scale = scale;
            z_rot.pose.pos = pos;
            z_rot.render(view, true);
        }

    } else if(active == Widget_Type::scale) {

        x_scl.pose.scale = scale;
        x_scl.pose.pos = pos + Vec3(0.15f * scl, 0.0f, 0.0f);
        x_scl.render(view, true);

        y_scl.pose.scale = scale;
        y_scl.pose.pos = pos + Vec3(0.0f, 0.15f * scl, 0.0f);
        y_scl.render(view, true);

        z_scl.pose.scale = scale;
        z_scl.pose.pos = pos + Vec3(0.0f, 0.0f, 0.15f * scl);
        z_scl.render(view, true);
    }
}

Pose Widgets::apply_action(const Pose& pose) {

    Pose result = pose;
    Vec3 vaxis;
    vaxis[(int)axis] = 1.0f;

    switch(active) {
    case Widget_Type::move: {
        result.pos = pose.pos + drag_end - drag_start;
    } break;
    case Widget_Type::rotate: {
        Quat rot = Quat::axis_angle(vaxis, drag_end[(int)axis]);
        Quat combined = rot * pose.rotation_quat();
        result.euler = combined.to_euler();
    } break;
    case Widget_Type::scale: {
        result.scale = Vec3{1.0f};
        result.scale[(int)axis] = drag_end[(int)axis];
        Mat4 rot = pose.rotation_mat();
        Mat4 trans =
            Mat4::transpose(rot) * Mat4::scale(result.scale) * rot * Mat4::scale(pose.scale);
        result.scale = Vec3(trans[0][0], trans[1][1], trans[2][2]);
    } break;
    case Widget_Type::bevel: {
        Vec2 off = bevel_start - bevel_end;
        result.pos = 2.0f * Vec3(off.x, -off.y, 0.0f);
    } break;
    default: assert(false);
    }

    return result;
}

bool Widgets::to_axis(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3& hit) {

    Vec3 axis1;
    axis1[(int)axis] = 1.0f;
    Vec3 axis2;
    axis2[((int)axis + 1) % 3] = 1.0f;
    Vec3 axis3;
    axis3[((int)axis + 2) % 3] = 1.0f;

    Line select(cam_pos, dir);
    Line target(obj_pos, axis1);
    Plane l(obj_pos, axis2);
    Plane r(obj_pos, axis3);

    Vec3 hit1, hit2;
    bool hl = l.hit(select, hit1);
    bool hr = r.hit(select, hit2);
    if(!hl && !hr)
        return false;
    else if(!hl)
        hit = hit2;
    else if(!hr)
        hit = hit1;
    else
        hit = (hit1 - cam_pos).norm() > (hit2 - cam_pos).norm() ? hit2 : hit1;

    hit = target.closest(hit);
    return hit.valid();
}

bool Widgets::to_plane(Vec3 obj_pos, Vec3 cam_pos, Vec3 dir, Vec3 norm, Vec3& hit) {

    Line look(cam_pos, dir);
    Plane p(obj_pos, norm);
    return p.hit(look, hit);
}

bool Widgets::is_dragging() {
    return dragging;
}

bool Widgets::want_drag() {
    return start_dragging;
}

void Widgets::start_drag(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir) {

    start_dragging = false;
    dragging = true;

    Vec3 hit;
    Vec3 norm;
    norm[(int)axis] = 1.0f;

    if(active == Widget_Type::rotate) {

        if(to_plane(pos, cam, dir, norm, hit)) {
            drag_start = (hit - pos).unit();
            drag_end = Vec3{0.0f};
        }

    } else {

        bool good;

        if(drag_plane)
            good = to_plane(pos, cam, dir, norm, hit);
        else
            good = to_axis(pos, cam, dir, hit);

        if(!good) return;

        if(active == Widget_Type::bevel) {
            bevel_start = bevel_end = spos;
        }
        if(active == Widget_Type::move) {
            drag_start = drag_end = hit;
        } else {
            drag_start = hit;
            drag_end = Vec3{1.0f};
        }

        if(active != Widget_Type::bevel) generate_lines(pos);
    }
}

void Widgets::end_drag() {
    lines.clear();
    drag_start = drag_end = {};
    bevel_start = bevel_end = {};
    dragging = false;
    drag_plane = false;
}

void Widgets::drag_to(Vec3 pos, Vec3 cam, Vec2 spos, Vec3 dir, bool scale_invert) {

    Vec3 hit;
    Vec3 norm;
    norm[(int)axis] = 1.0f;

    if(active == Widget_Type::bevel) {

        bevel_end = spos;

    } else if(active == Widget_Type::rotate) {

        if(!to_plane(pos, cam, dir, norm, hit)) return;

        Vec3 ang = (hit - pos).unit();
        float sgn = sign(cross(drag_start, ang)[(int)axis]);
        drag_end = Vec3{};
        drag_end[(int)axis] = sgn * Degrees(std::acos(dot(drag_start, ang)));

    } else {

        bool good;

        if(drag_plane)
            good = to_plane(pos, cam, dir, norm, hit);
        else
            good = to_axis(pos, cam, dir, hit);

        if(!good) return;

        if(active == Widget_Type::move) {
            drag_end = hit;
        } else if(active == Widget_Type::scale) {
            drag_end = Vec3{1.0f};
            drag_end[(int)axis] = (hit - pos).norm() / (drag_start - pos).norm();
        } else
            assert(false);
    }

    if(scale_invert && active == Widget_Type::scale) {
        drag_end[(int)axis] *= sign(dot(hit - pos, drag_start - pos));
    }
}

void Widgets::select(Scene_ID id) {

    start_dragging = true;
    drag_plane = false;

    switch(id) {
    case(Scene_ID)Widget_IDs::x_mov: {
        active = Widget_Type::move;
        axis = Axis::X;
    } break;
    case(Scene_ID)Widget_IDs::y_mov: {
        active = Widget_Type::move;
        axis = Axis::Y;
    } break;
    case(Scene_ID)Widget_IDs::z_mov: {
        active = Widget_Type::move;
        axis = Axis::Z;
    } break;
    case(Scene_ID)Widget_IDs::xy_mov: {
        active = Widget_Type::move;
        axis = Axis::Z;
        drag_plane = true;
    } break;
    case(Scene_ID)Widget_IDs::yz_mov: {
        active = Widget_Type::move;
        axis = Axis::X;
        drag_plane = true;
    } break;
    case(Scene_ID)Widget_IDs::xz_mov: {
        active = Widget_Type::move;
        axis = Axis::Y;
        drag_plane = true;
    } break;
    case(Scene_ID)Widget_IDs::x_rot: {
        active = Widget_Type::rotate;
        axis = Axis::X;
    } break;
    case(Scene_ID)Widget_IDs::y_rot: {
        active = Widget_Type::rotate;
        axis = Axis::Y;
    } break;
    case(Scene_ID)Widget_IDs::z_rot: {
        active = Widget_Type::rotate;
        axis = Axis::Z;
    } break;
    case(Scene_ID)Widget_IDs::x_scl: {
        active = Widget_Type::scale;
        axis = Axis::X;
    } break;
    case(Scene_ID)Widget_IDs::y_scl: {
        active = Widget_Type::scale;
        axis = Axis::Y;
    } break;
    case(Scene_ID)Widget_IDs::z_scl: {
        active = Widget_Type::scale;
        axis = Axis::Z;
    } break;
    default: {
        start_dragging = false;
    } break;
    }
}

void Widget_Camera::ar(Camera& user_cam, float _ar) {
    cam_ar = _ar;
    update_cameras(user_cam);
}

bool Widget_Camera::UI(Undo& undo, Camera& user_cam) {

    bool update_cam = false;
    bool do_undo = false;

    ImGui::Text("Camera Settings");
    if(moving_camera) {
        if(ImGui::Button("Confirm Move")) {
            moving_camera = false;
            old = render_cam;
            render_cam = user_cam;
            user_cam.set_ar(screen_dim);
            user_cam.set_fov(90.0f);
            update_cam = true;
            do_undo = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel Move")) {
            moving_camera = false;
            user_cam = saved_cam;
            user_cam.set_ar(screen_dim);
            user_cam.set_fov(90.0f);
        }
    } else {
        if(ImGui::Button("Free Move")) {
            moving_camera = true;
            user_cam = render_cam;
            saved_cam = render_cam;
        }
        ImGui::SameLine();
        if(ImGui::Button("Move to View")) {
            old = render_cam;
            render_cam = user_cam;
            update_cam = true;
            do_undo = true;
            cam_fov = user_cam.get_fov();
            cam_ar = user_cam.get_ar();
        }
    }
    if(ImGui::Button("Reset")) {
        old = render_cam;
        cam_fov = 90.0f;
        cam_ar = 1.7778f;
        cam_ap = 0.0f;
        cam_dist = 1.0f;
        update_cam = true;
        do_undo = true;
    }

    update_cam |= ImGui::SliderFloat("Aspect Ratio", &cam_ar, 0.1f, 10.0f, "%.2f");

    if(ImGui::IsItemActivated()) {
        old = render_cam;
        old_ar = cam_ar;
    }
    if(ImGui::IsItemDeactivated() && old_ar != cam_ar) do_undo = true;

    update_cam |= ImGui::SliderFloat("FOV", &cam_fov, 10.0f, 160.0f, "%.2f");

    if(ImGui::IsItemActivated()) {
        old = render_cam;
        old_fov = cam_fov;
    }
    if(ImGui::IsItemDeactivated() && old_fov != cam_fov) do_undo = true;

    update_cam |= ImGui::SliderFloat("Aperture", &cam_ap, 0.0f, 0.2f, "%.3f");
    if(ImGui::IsItemActivated()) {
        old = render_cam;
        old_ap = cam_ap;
    }
    if(ImGui::IsItemDeactivated() && old_ap != cam_ap) do_undo = true;

    update_cam |= ImGui::SliderFloat("Focal Distance", &cam_dist, 0.2f, 10.0f, "%.2f");
    if(ImGui::IsItemActivated()) {
        old = render_cam;
        old_dist = cam_dist;
    }
    if(ImGui::IsItemDeactivated() && old_dist != cam_dist) do_undo = true;

    cam_ar = clamp(cam_ar, 0.1f, 10.0f);
    cam_fov = clamp(cam_fov, 10.0f, 160.0f);
    cam_ap = clamp(cam_ap, 0.0f, 1.0f);
    cam_dist = clamp(cam_dist, 0.01f, 100.0f);

    if(update_cam) update_cameras(user_cam);
    if(do_undo) undo.update_camera(*this, old);

    return update_cam;
}

void Widget_Camera::update_cameras(Camera& user_cam) {
    render_cam.set_ar(cam_ar);
    render_cam.set_fov(cam_fov);
    render_cam.set_ap(cam_ap);
    render_cam.set_dist(cam_dist);
    if(moving_camera) {
        user_cam.set_ar(cam_ar);
        user_cam.set_fov(cam_fov);
        user_cam.set_ap(cam_fov);
        user_cam.set_dist(cam_dist);
    }
    generate_cage();
}

void Widget_Camera::load(Camera c) {
    render_cam.look_at(c.center(), c.pos());
    render_cam.set_ar(c.get_ar());
    render_cam.set_fov(c.get_fov());
    render_cam.set_ap(c.get_ap());
    render_cam.set_dist(c.get_dist());
    cam_fov = c.get_fov();
    cam_ar = c.get_ar();
    cam_ap = c.get_ap();
    cam_dist = c.get_dist();
    generate_cage();
}

void Widget_Camera::render(const Mat4& view) {
    if(!moving_camera) Renderer::get().lines(cam_cage, view);
}

void Widget_Camera::generate_cage() {
    cam_cage.clear();

    float ar = render_cam.get_ar();
    float fov = render_cam.get_fov();
    float h = 2.0f * std::tan(Radians(fov) / 2.0f);
    float w = ar * h;

    Mat4 iview = render_cam.get_view().inverse();

    Vec3 tr = iview * (Vec3(0.5f * w, 0.5f * h, -1.0f) * cam_dist);
    Vec3 tl = iview * (Vec3(-0.5f * w, 0.5f * h, -1.0f) * cam_dist);
    Vec3 br = iview * (Vec3(0.5f * w, -0.5f * h, -1.0f) * cam_dist);
    Vec3 bl = iview * (Vec3(-0.5f * w, -0.5f * h, -1.0f) * cam_dist);

    Vec3 ftr = iview * Vec3(0.5f * cam_ap, 0.5f * cam_ap, 0.0f);
    Vec3 ftl = iview * Vec3(-0.5f * cam_ap, 0.5f * cam_ap, 0.0f);
    Vec3 fbr = iview * Vec3(0.5f * cam_ap, -0.5f * cam_ap, 0.0f);
    Vec3 fbl = iview * Vec3(-0.5f * cam_ap, -0.5f * cam_ap, 0.0f);

    cam_cage.add(ftl, ftr, Gui::Color::black);
    cam_cage.add(ftr, fbr, Gui::Color::black);
    cam_cage.add(fbr, fbl, Gui::Color::black);
    cam_cage.add(fbl, ftl, Gui::Color::black);

    cam_cage.add(ftr, tr, Gui::Color::black);
    cam_cage.add(ftl, tl, Gui::Color::black);
    cam_cage.add(fbr, br, Gui::Color::black);
    cam_cage.add(fbl, bl, Gui::Color::black);

    cam_cage.add(bl, tl, Gui::Color::black);
    cam_cage.add(tl, tr, Gui::Color::black);
    cam_cage.add(tr, br, Gui::Color::black);
    cam_cage.add(br, bl, Gui::Color::black);
}

Widget_Render::Widget_Render(Vec2 dim) : pathtracer(*this, dim) {
    out_w = (size_t)dim.x / 2;
    out_h = (size_t)dim.y / 2;
}

void Widget_Render::open() {
    render_window = true;
    render_window_focus = true;
}

void Widget_Render::log_ray(const Ray& ray, float t, Spectrum color) {
    std::lock_guard<std::mutex> lock(log_mut);
    ray_log.add(ray.point, ray.at(t), Vec3(color.r, color.g, color.b));
}

void Widget_Render::begin(Scene& scene, Widget_Camera& cam, Camera& user_cam) {

    if(render_window_focus) {
        ImGui::SetNextWindowFocus();
        render_window_focus = false;
    }
    ImGui::SetNextWindowSize({675.0f, 625.0f}, ImGuiCond_Once);
    ImGui::Begin("Render Image", &render_window, ImGuiWindowFlags_NoCollapse);

    static const char* method_names[] = {"Rasterize", "Path Trace"};
    ImGui::Combo("Method", &method, method_names, 2);

    ImGui::InputInt("Width", &out_w, 1, 100);
    ImGui::InputInt("Height", &out_h, 1, 100);

    if(method == 1) {
        ImGui::InputInt("Samples", &out_samples, 1, 100);
        ImGui::InputInt("Area Light Samples", &out_area_samples, 1, 100);
        ImGui::InputInt("Max Ray Depth", &out_depth, 1, 32);
        ImGui::SliderFloat("Exposure", &exposure, 0.01f, 10.0f, "%.2f", 2.5f);
    } else {
        ImGui::Combo("Samples", (int*)&msaa.samples, GL::Sample_Count_Names, msaa.n_options());
        out_samples = msaa.n_samples();
    }

    out_w = std::max(1, out_w);
    out_h = std::max(1, out_h);
    out_samples = std::max(1, out_samples);
    out_area_samples = std::max(1, out_area_samples);
    out_depth = std::max(1, out_depth);

    if(ImGui::Button("Set Width via AR")) {
        out_w = (size_t)std::ceil(cam.get_ar() * out_h);
    }
    ImGui::SameLine();
    if(ImGui::Button("Set AR via W/H")) {
        cam.ar(user_cam, (float)out_w / (float)out_h);
    }
}

std::string Widget_Render::step(Animate& animate, Scene& scene) {

    if(animating) {

        if(next_frame == max_frame) {
            animating = false;
            return {};
        }
        if(folder.empty()) {
            animating = false;
            return "No output folder!";
        }

        Camera cam = animate.set_time(scene, (float)next_frame);
        animate.step_sim(scene);

        if(method == 0) {
            std::vector<unsigned char> data;

            Renderer::get().save(scene, cam, out_w, out_h, out_samples);
            Renderer::get().saved(data);

            std::stringstream str;
            str << std::setfill('0') << std::setw(4) << next_frame;
#ifdef _WIN32
            std::string path = folder + "\\" + str.str() + ".png";
#else
            std::string path = folder + "/" + str.str() + ".png";
#endif

            stbi_flip_vertically_on_write(true);
            if(!stbi_write_png(path.c_str(), (int)out_w, (int)out_h, 4, data.data(),
                               (int)out_w * 4)) {
                animating = false;
                return "Failed to write output!";
            }

            next_frame++;
        } else {

            if(init) {
                pathtracer.begin_render(scene, cam);
                init = false;
            }

            if(!pathtracer.in_progress()) {
                std::vector<unsigned char> data;

                pathtracer.get_output().tonemap_to(data, exposure);
                std::stringstream str;
                str << std::setfill('0') << std::setw(4) << next_frame;
#ifdef _WIN32
                std::string path = folder + "\\" + str.str() + ".png";
#else
                std::string path = folder + "/" + str.str() + ".png";
#endif

                stbi_flip_vertically_on_write(false);
                if(!stbi_write_png(path.c_str(), (int)out_w, (int)out_h, 4, data.data(),
                                   (int)out_w * 4)) {
                    animating = false;
                    return "Failed to write output!";
                }

                pathtracer.begin_render(scene, cam);
                next_frame++;
            }
        }
    }
    return {};
}

void Widget_Render::animate(Scene& scene, Widget_Camera& cam, Camera& user_cam, int last_frame) {

    if(!render_window) return;

    begin(scene, cam, user_cam);

    if(ImGui::Button("Output Folder")) {
        char* path = nullptr;
        NFD_OpenDirectoryDialog(nullptr, nullptr, &path);
        if(path) {
            Platform::strcpy(output_path, path, sizeof(output_path));
            free(path);
        }
    }
    ImGui::SameLine();
    ImGui::InputText("##path", output_path, sizeof(output_path));

    ImGui::Separator();
    ImGui::Text("Render");

    if(animating) {

        if(ImGui::Button("Cancel")) {
            pathtracer.cancel();
            animating = false;
        }

        ImGui::SameLine();
        if(method == 1) {
            ImGui::ProgressBar(((float)next_frame + pathtracer.progress()) / (max_frame + 1));
        } else {
            ImGui::ProgressBar((float)next_frame / (max_frame + 1));
        }

    } else {

        if(ImGui::Button("Start Render")) {
            animating = true;
            max_frame = last_frame;
            next_frame = 0;
            folder = std::string(output_path);
            if(method == 1) {
                init = true;
                ray_log.clear();
                pathtracer.set_sizes(out_w, out_h, out_samples, out_area_samples, out_depth);
            }
        }
    }

    float avail = ImGui::GetContentRegionAvail().x;
    float w = std::min(avail, (float)out_w);
    float h = (w / out_w) * out_h;

    if(method == 1) {
        ImGui::Image((ImTextureID)(long long)pathtracer.get_output_texture(exposure).get_id(),
                     {w, h});
    } else {
        ImGui::Image((ImTextureID)(long long)Renderer::get().saved(), {w, h}, {0.0f, 1.0f},
                     {1.0f, 0.0f});
    }

    ImGui::End();
}

static bool postfix(const std::string& path, const std::string& type) {
    if(path.length() >= type.length())
        return path.compare(path.length() - type.length(), type.length(), type) == 0;
    return false;
}

bool Widget_Render::UI(Scene& scene, Widget_Camera& cam, Camera& user_cam, std::string& err) {

    bool ret = false;
    if(!render_window) return ret;

    begin(scene, cam, user_cam);

    ImGui::Separator();
    ImGui::Text("Render");

    if(pathtracer.in_progress()) {

        if(ImGui::Button("Cancel")) {
            pathtracer.cancel();
        }

        ImGui::SameLine();
        ImGui::ProgressBar(pathtracer.progress());

    } else {

        if(ImGui::Button("Start Render")) {

            if(method == 1) {
                has_rendered = true;
                ret = true;
                ray_log.clear();
                pathtracer.set_sizes(out_w, out_h, out_samples, out_area_samples, out_depth);
                pathtracer.begin_render(scene, cam.get());
            } else {
                Renderer::get().save(scene, cam.get(), out_w, out_h, out_samples);
            }
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Save Image")) {
        char* path = nullptr;
        NFD_SaveDialog("png", nullptr, &path);
        if(path) {

            std::string spath(path);
            if(!postfix(spath, ".png")) {
                spath += ".png";
            }

            std::vector<unsigned char> data;

            if(method == 1) {
                pathtracer.get_output().tonemap_to(data, exposure);
                stbi_flip_vertically_on_write(false);
            } else {
                Renderer::get().saved(data);
                stbi_flip_vertically_on_write(true);
            }

            if(!stbi_write_png(spath.c_str(), (int)out_w, (int)out_h, 4, data.data(),
                               (int)out_w * 4)) {
                err = "Failed to write png!";
            }
            free(path);
        }
    }

    if(method == 1 && has_rendered) {
        ImGui::SameLine();
        if(ImGui::Button("Add Samples")) {
            pathtracer.begin_render(scene, cam.get(), true);
        }
    }

    float avail = ImGui::GetContentRegionAvail().x;
    float w = std::min(avail, (float)out_w);
    float h = (w / out_w) * out_h;

    if(method == 1) {
        ImGui::Image((ImTextureID)(long long)pathtracer.get_output_texture(exposure).get_id(),
                     {w, h});

        if(!pathtracer.in_progress() && has_rendered) {
            auto [build, render] = pathtracer.completion_time();
            ImGui::Text("Scene built in %.2fs, rendered in %.2fs.", build, render);
        }
    } else {
        ImGui::Image((ImTextureID)(long long)Renderer::get().saved(), {w, h}, {0.0f, 1.0f},
                     {1.0f, 0.0f});
    }

    ImGui::End();
    return ret;
}

std::string Widget_Render::headless(Animate& animate, Scene& scene, const Camera& cam,
                                    std::string output, bool a, int w, int h, int s, int ls, int d,
                                    float exp) {

    info("Render settings:");
    info("\twidth: %d", w);
    info("\theight: %d", h);
    info("\tsamples: %d", s);
    info("\tlight samples: %d", ls);
    info("\tmax depth: %d", d);
    info("\texposure: %f", exp);
    info("\trender threads: %u", std::thread::hardware_concurrency());

    out_w = w;
    out_h = h;
    pathtracer.set_sizes(w, h, s, ls, d);

    auto print_progress = [](float f) {
        std::cout << "Progress: [";

        int width = std::min(Platform::console_width() - 30, 50);
        if(width) {
            int bar = (int)(width * f);
            for(int i = 0; i < bar; i++) std::cout << "-";
            for(int i = bar; i < width; i++) std::cout << " ";
            std::cout << "] ";
        }

        float percent = 100.0f * f;
        if(percent < 10.0f) std::cout << "0";
        std::cout << percent << "%\r";
        std::cout.flush();
    };

    std::cout << std::fixed << std::setw(2) << std::setprecision(2) << std::setfill('0');
    if(a) {

        method = 1;
        init = true;
        animating = true;
        max_frame = animate.n_frames();
        next_frame = 0;
        folder = output;
        while(next_frame < max_frame) {
            std::string err = step(animate, scene);
            if(!err.empty()) return err;
            print_progress(((float)next_frame + pathtracer.progress()) / (max_frame + 1));
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        std::cout << std::endl;

    } else {

        pathtracer.begin_render(scene, cam);
        while(pathtracer.in_progress()) {
            print_progress(pathtracer.progress());
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        std::cout << std::endl;

        std::vector<unsigned char> data;
        pathtracer.get_output().tonemap_to(data, exp);
        if(!stbi_write_png(output.c_str(), w, h, 4, data.data(), w * 4)) {
            return "Failed to write output!";
        }
    }

    return {};
}

void Widget_Render::render_log(const Mat4& view) const {
    std::lock_guard<std::mutex> lock(log_mut);
    Renderer::get().lines(ray_log, view);
}

} // namespace Gui
