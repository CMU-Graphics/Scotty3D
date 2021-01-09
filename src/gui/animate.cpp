
#include "animate.h"
#include "../scene/renderer.h"
#include "manager.h"

#include <tuple>

namespace Gui {

Camera Anim_Camera::at(float t) const {
    Camera ret(dim);
    auto [p, r, f, a, ap, d] = splines.at(t);
    Vec3 dir = r.rotate(Vec3{0.0f, 0.0f, -1.0f});
    ret.look_at(p + dir, p);
    ret.set_fov(f);
    ret.set_ar(a);
    ret.set_ap(ap);
    ret.set_dist(d);
    return ret;
}

void Anim_Camera::set(float t, const Camera& cam) {
    splines.set(t, cam.pos(), Quat::euler(Mat4::rotate_z_to(cam.center() - cam.pos()).to_euler()),
                cam.get_fov(), cam.get_ar(), cam.get_ap(), cam.get_dist());
}

void Animate::update_dim(Vec2 dim) {
    ui_camera.dim(dim);
}

bool Animate::keydown(Widgets& widgets, Undo& undo, Scene_ID sel, SDL_Keysym key) {

    if(key.sym == SDLK_SPACE) {
        playing = !playing;
        last_frame = SDL_GetPerformanceCounter();
        return true;
    }

    return false;
}

Camera Animate::current_camera() const {
    return ui_camera.get();
}

void Animate::load_cam(Vec3 pos, Vec3 center, float ar, float hfov, float ap, float dist) {

    if(ar == 0.0f) ar = ui_render.wh_ar();

    float fov = 2.0f * std::atan((1.0f / ar) * std::tan(hfov / 2.0f));
    fov = Degrees(fov);

    Camera c(Vec2{ar, 1.0f});
    c.look_at(center, pos);
    c.set_ar(ar);
    c.set_fov(fov);
    c.set_ap(ap);
    c.set_dist(dist);
    ui_camera.load(c);
}

void Animate::render(Scene& scene, Scene_Maybe obj_opt, Widgets& widgets, Camera& user_cam) {

    Mat4 view = user_cam.get_view();
    auto& R = Renderer::get();

    ui_camera.render(view);

    if(visualize_splines)
        for(auto& e : spline_cache) R.lines(e.second, view);

    if(!obj_opt.has_value()) return;
    Scene_Item& item = obj_opt.value();

    if(item.is<Scene_Light>()) {
        Scene_Light& light = item.get<Scene_Light>();
        if(light.is_env()) return;
    }

    Pose& pose = item.pose();
    item.render(view);

    if(item.is<Scene_Object>() && item.get<Scene_Object>().armature.has_bones()) {

        Scene_Object& obj = item.get<Scene_Object>();
        joint_id_offset = scene.used_ids();
        obj.armature.render(view * obj.pose.transform(), joint_select, handle_select, false, true,
                            joint_id_offset);

        if(!joint_select && !handle_select) {
            R.begin_outline();
            BBox box = obj.bbox();
            obj.render(view, false, true);
            obj.armature.outline(view, obj.pose.transform(), false, true, box, joint_id_offset);
            R.end_outline(view, box);
        } else if(handle_select) {
            widgets.active = Widget_Type::move;
        } else {
            widgets.active = Widget_Type::rotate;
        }

    } else {
        R.outline(view, item);
    }

    if(handle_select) {

        Scene_Object& obj = item.get<Scene_Object>();
        Vec3 wpos = pose.transform() * (handle_select->target + obj.armature.base());
        float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);
        widgets.render(view, wpos, scale);

    } else if(joint_select) {

        Scene_Object& obj = item.get<Scene_Object>();
        Vec3 wpos = pose.transform() * obj.armature.posed_base_of(joint_select);
        float scale = std::min((user_cam.pos() - wpos).norm() / 5.5f, 10.0f);
        widgets.render(view, wpos, scale);

    } else {

        float scale = std::min((user_cam.pos() - pose.pos).norm() / 5.5f, 10.0f);
        widgets.render(view, pose.pos, scale);
    }
}

void Animate::make_spline(Scene_ID id, const Anim_Pose& pose) {

    if(!pose.splines.any()) return;

    auto entry = spline_cache.find(id);
    if(entry == spline_cache.end()) {
        std::tie(entry, std::ignore) = spline_cache.insert({id, GL::Lines()});
    }

    GL::Lines& lines = entry->second;
    lines.clear();

    Vec3 prev = pose.at(0.0f).pos;
    for(int i = 1; i < max_frame; i++) {

        float f = (float)i;
        float c = (float)(i % 20) / 19.0f;
        Vec3 cur = pose.at(f).pos;
        lines.add(prev, cur, Vec3{c, c, 1.0f});
        prev = cur;
    }
}

void Animate::camera_spline() {

    auto entry = spline_cache.find(0);
    if(entry == spline_cache.end()) {
        std::tie(entry, std::ignore) = spline_cache.insert({0, GL::Lines()});
    }

    GL::Lines& lines = entry->second;
    lines.clear();

    Vec3 prev = anim_camera.at(0.0f).pos();
    for(int i = 1; i < max_frame; i++) {
        float f = (float)i;
        float c = (float)(i % 20) / 19.0f;
        Vec3 cur = anim_camera.at(f).pos();
        lines.add(prev, cur, Vec3{c, c, 1.0f});
        prev = cur;
    }
}

void Animate::UIsidebar(Manager& manager, Undo& undo, Scene_Maybe obj_opt, Camera& user_cam) {

    if(!obj_opt.has_value()) {
        handle_select = nullptr;
        joint_select = nullptr;
    }

    if(handle_select) {

        Scene_ID id = obj_opt.value().get().id();

        ImGui::Text("Edit IK Handle");
        ImGui::DragFloat3("Pos", handle_select->target.data, 0.1f, 0.0f, 0.0f, "%.2f");
        if(ImGui::IsItemActivated()) old_euler = handle_select->target;
        if(ImGui::IsItemDeactivatedAfterEdit() && old_euler != handle_select->target) {
            undo.move_handle(id, handle_select, old_euler);
        }
        ImGui::Checkbox("Enable", &handle_select->enabled);
        ImGui::Separator();

    } else if(joint_select) {

        Scene_Item& item = obj_opt.value().get();
        Scene_ID id = item.id();

        ImGui::Text("Edit Joint");

        if(ImGui::DragFloat3("Pose", joint_select->pose.data, 1.0f, 0.0f, 0.0f, "%.2f"))
            obj_opt.value().get().get<Scene_Object>().set_pose_dirty();
        if(ImGui::IsItemActivated()) old_euler = joint_select->pose;
        if(ImGui::IsItemDeactivatedAfterEdit() && old_euler != joint_select->pose) {
            joint_select->pose = joint_select->pose.range(0.0f, 360.0f);
            undo.pose_bone(id, joint_select, old_euler);
        }
        ImGui::Separator();
    }

    if(obj_opt.has_value()) {
        Scene_Item& item = obj_opt.value().get();
        if(item.is<Scene_Object>()) {
            Scene_Object& obj = item.get<Scene_Object>();
            if(obj.armature.has_bones()) {
                if(obj.armature.do_ik()) obj.set_pose_dirty();
            }
        }
    }

    if(ui_camera.UI(undo, user_cam)) {
        camera_selected = true;
        if(obj_opt.has_value()) prev_selected = obj_opt.value().get().id();
    }
}

void Animate::end_transform(Undo& undo, Scene_Item& obj) {
    if(handle_select) {
        undo.move_handle(obj.id(), handle_select, old_euler);
    } else if(joint_select) {
        undo.pose_bone(obj.id(), joint_select, old_euler);
    } else {
        undo.update_pose(obj.id(), old_pose);
    }
    old_pose = {};
    old_euler = {};
    old_T = Mat4::I;
}

Vec3 Animate::selected_pos(Scene_Item& item) {
    if(handle_select) {
        return item.pose().transform() *
               (handle_select->target + item.get<Scene_Object>().armature.base());
    } else if(joint_select) {
        return item.pose().transform() *
               item.get<Scene_Object>().armature.posed_base_of(joint_select);
    }
    return item.pose().pos;
}

void Animate::apply_transform(Widgets& widgets, Scene_Item& item) {
    if(handle_select) {

        Scene_Object& obj = item.get<Scene_Object>();
        Vec3 p = old_T * widgets.apply_action(old_pose).pos;
        handle_select->target = p - obj.armature.base();

    } else if(joint_select) {
        Scene_Object& obj = item.get<Scene_Object>();
        Vec3 euler = widgets.apply_action(old_pose).euler;
        joint_select->pose = (old_T * Mat4::euler(euler)).to_euler();
        obj.set_pose_dirty();
    } else {
        item.pose() = widgets.apply_action(old_pose);
    }
}

bool Animate::select(Scene& scene, Widgets& widgets, Scene_ID selected, Scene_ID id, Vec3 cam,
                     Vec2 spos, Vec3 dir) {

    if(widgets.want_drag()) {

        if(handle_select) {

            Scene_Object& obj = scene.get_obj(selected);
            Vec3 base = obj.pose.transform() * (handle_select->target + obj.armature.base());
            widgets.start_drag(base, cam, spos, dir);
            old_pose = {};
            old_pose.pos = base;
            old_euler = handle_select->target;
            old_T = obj.pose.transform().inverse();

        } else if(joint_select) {

            Scene_Object& obj = scene.get_obj(selected);
            Vec3 base = obj.pose.transform() * obj.armature.posed_base_of(joint_select);
            widgets.start_drag(base, cam, spos, dir);

            Mat4 j_to_p = obj.pose.transform() * obj.armature.joint_to_posed(joint_select);
            old_euler = joint_select->pose;
            old_pose.euler = j_to_p.to_euler();
            j_to_p = j_to_p * Mat4::euler(old_euler).inverse();
            old_T = j_to_p.inverse();

        } else {

            Scene_Item& item = scene.get(selected).value();
            Pose& pose = item.pose();
            widgets.start_drag(pose.pos, cam, spos, dir);
            old_pose = pose;
        }

        return false;
    }

    Scene_Maybe mb = scene.get(selected);
    if(mb.has_value()) {

        Scene_Item& item = mb.value().get();
        if(item.is<Scene_Object>()) {

            Scene_Object& obj = item.get<Scene_Object>();
            if(id >= joint_id_offset && obj.armature.has_bones()) {

                Scene_ID j_id = id - joint_id_offset;
                joint_select = obj.armature.get_joint(j_id);
                handle_select = obj.armature.get_handle(j_id);
                if(joint_select) {
                    widgets.active = Widget_Type::rotate;
                } else if(handle_select) {
                    widgets.active = Widget_Type::move;
                }
                return false;
            }
        }
    }

    if(id >= n_Widget_IDs) {
        if(id == selected) {
            joint_select = nullptr;
            handle_select = nullptr;
        }
        return true;
    }

    joint_select = nullptr;
    handle_select = nullptr;
    return false;
}

void Animate::timeline(Manager& manager, Undo& undo, Scene& scene, Scene_Maybe obj,
                       Camera& user_cam) {

    // NOTE(max): this is pretty messy
    //      Would be good to add the ability to set per-component keyframes
    //      ^ I started with that but it was hard to make work with assimp and
    //        generally made everything a lot messier.

    ImVec2 size = ImGui::GetWindowSize();

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 150.0f);

    if(!playing) {
        if(ImGui::Button("Play")) {
            playing = true;
            last_frame = SDL_GetPerformanceCounter();
        }
    } else {
        if(ImGui::Button("Stop")) {
            playing = false;
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Render")) {
        ui_render.open();
    }
    ui_render.animate(scene, ui_camera, user_cam, max_frame);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
    ImGui::Dummy({1.0f, 4.0f});
    ImGui::PopStyleVar();

    if(ImGui::Button("Add Frames")) {
        max_frame += 3 * frame_rate;
    }

    auto crop_item = [&, this](Scene_Item& item) {
        undo.anim_crop(item.id(), (float)max_frame);
        make_spline(item.id(), item.animation());
    };

    if(ImGui::Button("Crop End")) {
        size_t n = undo.n_actions();
        undo.set_max_frame(*this, max_frame, current_frame + 1);
        max_frame = current_frame + 1;
        current_frame = std::min(current_frame, max_frame - 1);
        undo.anim_crop_camera(anim_camera, (float)max_frame);
        scene.for_items(crop_item);
        undo.bundle_last(undo.n_actions() - n);
        set_time(scene, (float)current_frame);
    }

    ImGui::SliderInt("Rate", &frame_rate, 1, 240);
    frame_rate = clamp(frame_rate, 1, 240);

    ImGui::Checkbox("Draw Splines", &visualize_splines);

    ImGui::NextColumn();

    Scene_Item* select = nullptr;
    if(obj.has_value()) {
        Scene_Item& item = obj.value();
        select = &item;
        if(item.id() != prev_selected) {
            camera_selected = false;
            prev_selected = item.id();
        }
    }

    bool frame_changed = false;

    ImGui::Text("Keyframe:");
    ImGui::SameLine();

    auto set_item = [&, this](Scene_Item& item) {
        if(item.is<Scene_Object>()) {
            undo.anim_object(item.id(), (float)current_frame);
        }
        if(item.is<Scene_Light>()) {
            undo.anim_light(item.id(), (float)current_frame);
        }
        if(item.is<Scene_Particles>()) {
            undo.anim_particles(item.id(), (float)current_frame);
        }
        make_spline(item.id(), item.animation());
    };
    auto clear_item = [&, this](Scene_Item& item) {
        if(item.is<Scene_Object>()) {
            undo.anim_clear_object(item.id(), (float)current_frame);
        }
        if(item.is<Scene_Light>()) {
            undo.anim_clear_light(item.id(), (float)current_frame);
        }
        if(item.is<Scene_Particles>()) {
            undo.anim_clear_particles(item.id(), (float)current_frame);
        }
        make_spline(item.id(), item.animation());
    };

    if(ImGui::Button("Set")) {
        if(camera_selected) {
            undo.anim_camera(anim_camera, (float)current_frame, ui_camera.get());
            camera_spline();
        } else if(select) {
            set_item(*select);
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Clear")) {
        if(camera_selected) {
            undo.anim_clear_camera(anim_camera, (float)current_frame);
            camera_spline();
        } else if(select) {
            clear_item(*select);
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Set All")) {
        size_t n = undo.n_actions();
        undo.anim_camera(anim_camera, (float)current_frame, ui_camera.get());
        camera_spline();
        scene.for_items(set_item);
        undo.bundle_last(undo.n_actions() - n);
    }

    ImGui::SameLine();
    if(ImGui::Button("Clear All")) {
        size_t n = undo.n_actions();
        undo.anim_clear_camera(anim_camera, (float)current_frame);
        camera_spline();
        scene.for_items(clear_item);
        undo.bundle_last(undo.n_actions() - n);
    }

    ImGui::Separator();
    ImGui::Dummy({74.0f, 1.0f});
    ImGui::SameLine();
    if(ImGui::SliderInt("Frame", &current_frame, 0, max_frame - 1)) {
        frame_changed = true;
    }
    ImGui::BeginChild("Timeline", {size.x - 20.0f, size.y - 80.0f}, false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    const int name_chars = 12;
    std::vector<bool> frames;
    std::vector<Scene_ID> live_ids;

    {
        frames.clear();
        frames.resize(max_frame);

        std::string name = "Camera";
        ImVec2 sz = ImGui::CalcTextSize(name.c_str());
        if(camera_selected)
            ImGui::TextColored({Color::outline.x, Color::outline.y, Color::outline.z, 1.0f}, "%s",
                               name.c_str());
        else
            ImGui::Text("%s", name.c_str());
        ImGui::SameLine();
        ImGui::Dummy({80.0f - sz.x, 1.0f});
        ImGui::SameLine();
        ImGui::PushID("##CAMERA_");

        std::set<float> keys = anim_camera.splines.keys();

        for(float f : keys) {
            int frame = (int)std::round(f);
            if(frame >= 0 && frame < max_frame) frames[frame] = true;
        }

        for(int i = 0; i < max_frame; i++) {
            if(i > 0) ImGui::SameLine();
            ImGui::PushID(i);

            bool color = false;
            std::string label = "_";

            if(i == current_frame) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
                color = true;
            }

            if(frames[i]) {
                label = "*";
                if(i != current_frame) {
                    color = true;
                    ImGui::PushStyleColor(ImGuiCol_Button,
                                          ImGui::GetColorU32(ImGuiCol_ButtonHovered));
                }
            }
            if(ImGui::SmallButton(label.c_str())) {
                current_frame = i;
                frame_changed = true;
                camera_selected = true;
            }
            if(color) ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    scene.for_items([&, this](Scene_Item& item) {
        frames.clear();
        frames.resize(max_frame);

        std::string name = const_cast<const Scene_Item&>(item).name();
        name.resize(name_chars);

        ImVec2 size = ImGui::CalcTextSize(name.c_str());
        if(!camera_selected && select && item.id() == select->id())
            ImGui::TextColored({Color::outline.x, Color::outline.y, Color::outline.z, 1.0f}, "%s",
                               name.c_str());
        else
            ImGui::Text("%s", name.c_str());
        ImGui::SameLine();
        ImGui::Dummy({80.0f - size.x, 1.0f});
        ImGui::SameLine();

        ImGui::PushID(item.id());

        Anim_Pose animation = item.animation();
        std::set<float> keys = animation.splines.keys();
        if(item.is<Scene_Light>()) {
            std::set<float> more_keys = item.get<Scene_Light>().lanim.splines.keys();
            keys.insert(more_keys.begin(), more_keys.end());
        }
        if(item.is<Scene_Object>()) {
            std::set<float> more_keys = item.get<Scene_Object>().armature.keys();
            keys.insert(more_keys.begin(), more_keys.end());
        }
        if(item.is<Scene_Particles>()) {
            std::set<float> more_keys = item.get<Scene_Particles>().panim.splines.keys();
            keys.insert(more_keys.begin(), more_keys.end());
        }

        for(float f : keys) {
            int frame = (int)std::round(f);
            if(frame >= 0 && frame < max_frame) frames[frame] = true;
        }

        for(int i = 0; i < max_frame; i++) {
            if(i > 0) ImGui::SameLine();
            ImGui::PushID(i);

            bool color = false;
            std::string label = "_";

            if(i == current_frame) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
                color = true;
            }

            if(frames[i]) {
                label = "*";
                if(i != current_frame) {
                    color = true;
                    ImGui::PushStyleColor(ImGuiCol_Button,
                                          ImGui::GetColorU32(ImGuiCol_ButtonHovered));
                }
            }

            if(ImGui::SmallButton(label.c_str())) {
                current_frame = i;
                frame_changed = true;
                camera_selected = false;
                manager.set_select(item.id());
            }
            if(color) ImGui::PopStyleColor();
            ImGui::PopID();
        }

        live_ids.push_back(item.id());

        ImGui::SameLine();
        ImGui::Dummy({142.0f, 1.0f});
        ImGui::PopID();
    });

    ImGui::PopStyleVar();
    ImGui::EndChild();

    std::unordered_map<Scene_ID, GL::Lines> new_cache;
    for(Scene_ID i : live_ids) {
        auto entry = spline_cache.find(i);
        if(entry != spline_cache.end()) {
            new_cache[i] = std::move(entry->second);
        }
    }
    spline_cache = std::move(new_cache);

    if(frame_changed) update(scene);
}

void Animate::step_sim(Scene& scene) {
    simulate.step(scene, 1.0f / frame_rate);
}

Camera Animate::set_time(Scene& scene, float time) {

    current_frame = (int)time;

    scene.for_items([time](Scene_Item& item) { item.set_time(time); });

    Camera cam = anim_camera.at(time);
    if(anim_camera.splines.any()) {
        ui_camera.load(cam);
    } else {
        cam = ui_camera.get();
    }

    simulate.build_scene(scene);
    return cam;
}

void Animate::set_max(int frames) {
    max_frame = frames;
    current_frame = std::min(current_frame, max_frame - 1);
}

void Animate::set(int n_frames, int fps) {
    max_frame = n_frames;
    frame_rate = fps;
    current_frame = std::min(current_frame, max_frame - 1);
}

Anim_Camera& Animate::camera() {
    return anim_camera;
}

const Anim_Camera& Animate::camera() const {
    return anim_camera;
}

float Animate::fps() const {
    return (float)frame_rate;
}

int Animate::n_frames() const {
    return max_frame;
}

std::string Animate::pump_output(Scene& scene) {
    return ui_render.step(*this, scene);
}

void Animate::refresh(Scene& scene) {
    set_time(scene, (float)current_frame);
}

void Animate::clear() {
    anim_camera.splines.clear();
    joint_select = nullptr;
    handle_select = nullptr;
}

void Animate::invalidate(Skeleton::IK_Handle* handle) {
    if(handle_select == handle) handle_select = nullptr;
}

void Animate::invalidate(Joint* j) {
    if(joint_select == j) joint_select = nullptr;
}

void Animate::update(Scene& scene) {

    Uint64 time = SDL_GetPerformanceCounter();

    if(playing) {
        if((time - last_frame) * frame_rate / SDL_GetPerformanceFrequency()) {
            if(current_frame == max_frame - 1) {
                playing = false;
                current_frame = 0;
            } else {
                current_frame++;
            }
            last_frame = time;
        }
    }

    if(displayed_frame != current_frame) {
        set_time(scene, (float)current_frame);
        displayed_frame = current_frame;
    }
}

} // namespace Gui
