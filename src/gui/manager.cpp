
#include <imgui/imgui.h>
#include <nfd/nfd.h>

#include "manager.h"

#include "../geometry/util.h"
#include "../scene/renderer.h"

namespace Gui {

Manager::Manager(Scene &scene, Vec2 dim)
    : render(scene, dim), animate(dim), baseplane(1.0f), window_dim(dim) {
    create_baseplane();
}

void Manager::update_dim(Vec2 dim) {
    window_dim = dim;
    render.update_dim(dim);
    animate.update_dim(dim);
}

Vec3 Color::axis(Axis a) {
    switch (a) {
    case Axis::X:
        return red;
    case Axis::Y:
        return green;
    case Axis::Z:
        return blue;
    default:
        assert(false);
    }
    return Vec3();
}

void Manager::invalidate_obj(Scene_ID id) {
    if (id == layout.selected()) {
        layout.clear_select();
        model.unset_mesh();
        rig.clear();
        animate.clear();
    }
}

bool Manager::keydown(Undo &undo, SDL_Keysym key, Scene &scene, Camera &cam) {

    if (widgets.is_dragging())
        return false;

#ifdef __APPLE__
    Uint16 mod = KMOD_GUI;
    if (key.sym == SDLK_BACKSPACE && key.mod & KMOD_GUI) {
#else
    Uint16 mod = KMOD_CTRL;
    if (key.sym == SDLK_DELETE && layout.selected()) {
#endif
        if (mode == Mode::model) {
            model.erase_selected(undo, scene.get(layout.selected()));
        } else if (mode != Mode::rig && mode != Mode::animate) {
            undo.del_obj(layout.selected());
            return true;
        }
    }

    if (key.mod & mod) {
        switch (key.sym) {
        case SDLK_d:
            debug_shown = true;
            return true;
        case SDLK_e:
            write_scene(scene);
            return true;
        case SDLK_o:
            load_scene(scene, undo, true);
            return true;
        case SDLK_s:
            save_scene(scene);
            return true;
        }
    }

    switch (key.sym) {
    case SDLK_m:
        widgets.active = Widget_Type::move;
        return true;
    case SDLK_r:
        widgets.active = Widget_Type::rotate;
        return true;
    case SDLK_s:
        widgets.active = Widget_Type::scale;
        return true;
    case SDLK_f: {
        if (mode == Mode::rig) {
            cam.look_at(Vec3{}, -cam.front() * cam.dist());
            return true;
        } else if (mode != Mode::model && layout.selected()) {
            frame(scene, cam);
            return true;
        }
    } break;
    }

    switch (mode) {
    case Mode::layout:
        return layout.keydown(widgets, key);
    case Mode::model:
        return model.keydown(widgets, key, cam);
    case Mode::render:
        return render.keydown(widgets, key);
    case Mode::rig:
        return rig.keydown(widgets, undo, key);
    case Mode::animate:
        return animate.keydown(widgets, undo, layout.selected(), key);
    case Mode::simulate:
        break;
    }
    return false;
}

void Manager::save_scene(Scene &scene) {
    if (save_file.empty()) {
        char *path = nullptr;
        NFD_SaveDialog("dae", nullptr, &path);
        if (path) {
            save_file = std::string(path);
            free(path);
        }
    }
    std::string error = scene.write(save_file, render.get_cam(), animate);
    set_error(error);
}

static bool postfix(const std::string &path, const std::string &type) {
    if (path.length() >= type.length())
        return path.compare(path.length() - type.length(), type.length(), type) == 0;
    return false;
}

void Manager::write_scene(Scene &scene) {

    char *path = nullptr;
    NFD_SaveDialog("dae", nullptr, &path);
    if (path) {
        std::string spath(path);
        if (!postfix(path, ".dae")) {
            spath += ".dae";
        }
        std::string error = scene.write(spath, render.get_cam(), animate);
        set_error(error);
        free(path);
    }
}

void Manager::set_file(std::string save) { save_file = save; }

void Manager::load_scene(Scene &scene, Undo &undo, bool clear) {

    char *path = nullptr;
    NFD_OpenDialog(scene_file_types, nullptr, &path);
    if (path) {
        if (clear) {
            save_file = std::string(path);
            layout.clear_select();
            model.unset_mesh();
        }
        std::string error = scene.load(clear, undo, *this, std::string(path));
        set_error(error);
        free(path);
    }
}

void Manager::load_image(Scene_Light &light) {

    char *path = nullptr;
    NFD_OpenDialog(image_file_types, nullptr, &path);
    if (path) {
        std::string error = light.emissive_load(std::string(path));
        set_error(error);
        free(path);
    }
    light.dirty();
}

void Manager::material_edit_gui(Undo &undo, Scene_ID obj_id, Material &material) {

    static Material::Options old_opt;
    Material::Options start_opt = material.opt;
    Material::Options &opt = material.opt;

    bool U = false;

    auto activate = [&]() {
        if (ImGui::IsItemDeactivated() && old_opt != opt)
            U = true;
        else if (ImGui::IsItemActivated())
            old_opt = start_opt;
    };

    if (ImGui::Combo("Type", (int *)&opt.type, Material_Type_Names, (int)Material_Type::count)) {
        if (start_opt != opt) {
            old_opt = start_opt;
            U = true;
        }
    }

    switch (opt.type) {
    case Material_Type::lambertian: {
        ImGui::ColorEdit3("Albedo", opt.albedo.data);
        activate();
    } break;
    case Material_Type::mirror: {
        ImGui::ColorEdit3("Reflectance", opt.reflectance.data);
        activate();
    } break;
    case Material_Type::refract: {
        ImGui::ColorEdit3("Transmittance", opt.transmittance.data);
        activate();
        ImGui::DragFloat("Index of Refraction", &opt.ior, 0.1f, 0.0f,
                         std::numeric_limits<float>::max(), "%.2f");
        activate();
    } break;
    case Material_Type::glass: {
        ImGui::ColorEdit3("Reflectance", opt.reflectance.data);
        activate();
        ImGui::ColorEdit3("Transmittance", opt.transmittance.data);
        activate();
        ImGui::DragFloat("Index of Refraction", &opt.ior, 0.1f, 0.0f,
                         std::numeric_limits<float>::max(), "%.2f");
        activate();
    } break;
    case Material_Type::diffuse_light: {
        ImGui::ColorEdit3("Emissive", opt.emissive.data);
        activate();
        ImGui::DragFloat("Intensity", &opt.intensity, 0.1f, 0.0f, std::numeric_limits<float>::max(),
                         "%.2f");
        activate();
    } break;
    default:
        break;
    }

    if (U) {
        undo.update_material(obj_id, old_opt);
    }
}

Mode Manager::item_options(Undo &undo, Mode cur_mode, Scene_Item &item, Pose &old_pose) {

    Pose &pose = item.pose();

    auto sliders = [&](Widget_Type act, std::string label, Vec3 &data, float sens) {
        if (ImGui::DragFloat3(label.c_str(), data.data, sens))
            widgets.active = act;
        if (ImGui::IsItemActivated())
            old_pose = pose;
        if (ImGui::IsItemDeactivatedAfterEdit() && old_pose != pose)
            undo.update_pose(item.id(), old_pose);
    };

    if (!(item.is<Scene_Light>() && item.get<Scene_Light>().is_env()) &&
        ImGui::CollapsingHeader("Edit Pose")) {
        ImGui::Indent();

        pose.clamp_euler();
        sliders(Widget_Type::move, "Position", pose.pos, 0.1f);
        sliders(Widget_Type::rotate, "Rotation", pose.euler, 1.0f);
        sliders(Widget_Type::scale, "Scale", pose.scale, 0.03f);

        widgets.action_button(Widget_Type::move, "Move [m]", false);
        widgets.action_button(Widget_Type::rotate, "Rotate [r]");
        widgets.action_button(Widget_Type::scale, "Scale [s]");
        if (Manager::wrap_button("Delete [del]")) {
            undo.del_obj(item.id());
        }

        ImGui::Unindent();
    }

    if (item.is<Scene_Object>()) {

        Scene_Object &obj = item.get<Scene_Object>();
        static Scene_Object::Options old_opt;
        Scene_Object::Options start_opt = obj.opt;

        bool U = false, E = false;
        auto activate = [&]() {
            if (ImGui::IsItemActive())
                E = true;
            if (ImGui::IsItemDeactivated() && old_opt != obj.opt)
                U = true;
            else if (ImGui::IsItemActivated())
                old_opt = start_opt;
        };
        auto update = [&]() {
            obj.set_mesh_dirty();
            undo.update_object(obj.id(), start_opt);
        };

        if ((obj.is_editable() || obj.is_shape()) && ImGui::CollapsingHeader("Edit Mesh")) {
            ImGui::Indent();
            if (obj.is_editable()) {
                if (ImGui::Button("Edit Mesh##button")) {
                    cur_mode = Mode::model;
                }
                ImGui::SameLine();
                if (ImGui::Button("Flip Normals")) {
                    obj.flip_normals();
                }
                if (ImGui::Checkbox("Smooth Normals", &obj.opt.smooth_normals)) {
                    update();
                }
                if (ImGui::Checkbox("Show Wireframe", &obj.opt.wireframe))
                    update();
            }
            if (ImGui::Combo("Use Implicit Shape", (int *)&obj.opt.shape_type, PT::Shape_Type_Names,
                             (int)PT::Shape_Type::count)) {
                if (obj.opt.shape_type == PT::Shape_Type::none)
                    obj.try_make_editable(start_opt.shape_type);
                update();
            }
            if (obj.opt.shape_type == PT::Shape_Type::sphere) {
                ImGui::DragFloat("Radius", &obj.opt.shape.get<PT::Sphere>().radius, 0.1f, 0.0f,
                                 std::numeric_limits<float>::max(), "%.2f");
                activate();
            }
            ImGui::Unindent();

            if (E)
                obj.set_mesh_dirty();
            if (U)
                undo.update_object(obj.id(), old_opt);
        }
        if (ImGui::CollapsingHeader("Edit Material")) {
            ImGui::Indent();
            material_edit_gui(undo, obj.id(), obj.material);
            ImGui::Unindent();
        }

    } else if (item.is<Scene_Light>()) {

        Scene_Light &light = item.get<Scene_Light>();

        if (ImGui::CollapsingHeader("Edit Light")) {
            ImGui::Indent();
            light_edit_gui(undo, light);
            ImGui::Unindent();
        }
    }
    return cur_mode;
}

void Manager::light_edit_gui(Undo &undo, Scene_Light &light) {

    static Scene_Light::Options old_opt;
    Scene_Light::Options start_opt = light.opt;

    bool E = false, U = false;

    auto activate = [&]() {
        if (ImGui::IsItemActive())
            E = true;
        if (ImGui::IsItemDeactivated() && old_opt != light.opt)
            U = true;
        else if (ImGui::IsItemActivated())
            old_opt = start_opt;
    };

    if (ImGui::Combo("Type", (int *)&light.opt.type, Light_Type_Names, (int)Light_Type::count)) {
        if (start_opt != light.opt) {
            old_opt = start_opt;
            U = true;
            E = true;
        }
    }

    if (!(light.opt.type == Light_Type::sphere && light.opt.has_emissive_map)) {
        ImGui::ColorEdit3("Spectrum", light.opt.spectrum.data);
        activate();

        ImGui::DragFloat("Intensity", &light.opt.intensity, 0.1f, 0.0f,
                         std::numeric_limits<float>::max(), "%.2f");
        activate();
    }

    switch (light.opt.type) {
    case Light_Type::sphere: {
        if (light.opt.has_emissive_map) {
            float x = ImGui::GetContentRegionAvail().x;
            ImGui::Image((ImTextureID)(long long)light.emissive_texture().get_id(), {x, x / 2.0f});
            if (ImGui::Button("Change Map")) {
                load_image(light);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Map")) {
                light.emissive_clear();
                old_opt = start_opt;
                U = true;
            }
        } else {
            if (ImGui::Button("Use Texture Map")) {
                load_image(light);
                if (light.opt.has_emissive_map) {
                    old_opt = start_opt;
                    U = true;
                }
            }
        }
    } break;
    case Light_Type::spot: {
        ImGui::DragFloat2("Angle Cutoffs", light.opt.angle_bounds.data, 1.0f, 0.0f, 360.0f);
        activate();
    } break;
    case Light_Type::rectangle: {
        ImGui::DragFloat2("Size", light.opt.size.data, 0.1f, 0.0f,
                          std::numeric_limits<float>::max());
        activate();
    } break;
    default:
        break;
    }

    if (E)
        light.dirty();
    if (U)
        undo.update_light(light.id(), old_opt);
}

bool Manager::wrap_button(std::string label) {
    ImGuiStyle &style = ImGui::GetStyle();
    float available_w = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    float last_w = ImGui::GetItemRectMax().x;
    float next_w = last_w + style.ItemSpacing.x + ImGui::CalcTextSize(label.c_str()).x +
                   style.FramePadding.x * 2;
    if (next_w < available_w)
        ImGui::SameLine();
    return ImGui::Button(label.c_str());
};

void Manager::frame(Scene &scene, Camera &cam) {
    if (!layout.selected())
        return;

    Vec3 center = layout.selected_pos(scene);
    Vec3 dir = cam.front() * cam.dist();
    cam.look_at(center, center - dir);
}

void Manager::UIsidebar(Scene &scene, Undo &undo, float menu_height, Camera &cam) {

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    static float anim_height = 0.0f;

    ImGui::SetNextWindowPos({0.0, menu_height});

    float h_cut = menu_height + (mode == Mode::animate ? anim_height : 0.0f);

    ImGui::SetNextWindowSizeConstraints({window_dim.x / 4.75f, window_dim.y - h_cut},
                                        {window_dim.x, window_dim.y - h_cut});
    ImGui::Begin("Menu", nullptr, flags);

    if (mode == Mode::layout) {
        ImGui::Text("Edit Scene");
        if (ImGui::Button("Import New Scene"))
            load_scene(scene, undo, true);
        if (wrap_button("Export Scene"))
            write_scene(scene);
        if (ImGui::Button("Import Objects"))
            load_scene(scene, undo, false);
        if (wrap_button("New Object")) {
            new_obj_window = true;
            new_obj_focus = true;
        }
        if (wrap_button("New Light")) {
            new_light_window = true;
            new_light_focus = true;
        }
    }

    ImGui::Separator();
    ImGui::Text("Select an Object");

    scene.for_items([&](Scene_Item &obj) {
        if ((mode == Mode::model || mode == Mode::rig) &&
            (!obj.is<Scene_Object>() || !obj.get<Scene_Object>().is_editable()))
            return;

        ImGui::PushID(obj.id());

        auto [name, cap] = obj.name();
        ImGui::InputText("##name", name, cap);

        bool is_selected = obj.id() == layout.selected();
        ImGui::SameLine();
        if (ImGui::Checkbox("##selected", &is_selected)) {
            if (is_selected)
                layout.set_selected(obj.id());
            else
                layout.clear_select();
        }

        ImGui::PopID();
    });
    if (!scene.empty()) {

        if (mode != Mode::model && mode != Mode::rig && layout.selected() &&
            ImGui::Button("Center Object [f]")) {
            frame(scene, cam);
        }

        ImGui::Separator();
    }

    auto selected = scene.get(layout.selected());

    switch (mode) {
    case Mode::layout: {
        mode = layout.UIsidebar(*this, undo, widgets, selected);
    } break;

    case Mode::model: {
        std::string err = model.UIsidebar(undo, widgets, selected, cam);
        set_error(err);
    } break;

    case Mode::render: {
        mode = render.UIsidebar(*this, undo, scene, selected, cam);
    } break;

    case Mode::rig: {
        mode = rig.UIsidebar(*this, undo, widgets, selected);
    } break;

    case Mode::animate: {
        if (layout.UIsidebar(*this, undo, widgets, selected) == Mode::model)
            mode = Mode::model;
        animate.UIsidebar(*this, undo, selected, cam);
        ImGui::End();
        ImGui::SetNextWindowPos({0.0, window_dim.y}, ImGuiCond_Always, {0.0f, 1.0f});
        ImGui::SetNextWindowSize({window_dim.x, window_dim.y / 4.0f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints({window_dim.x, window_dim.y / 4.0f}, window_dim);
        ImGui::Begin("Timeline", nullptr, flags);
        anim_height = ImGui::GetWindowHeight();
        animate.timeline(*this, undo, scene, selected, cam);
    } break;

    default:
        break;
    }

    ImGui::End();

    if (mode == Mode::layout) {
        if (new_obj_focus) {
            ImGui::SetNextWindowFocus();
            new_obj_focus = false;
        }
        if (new_obj_window) {
            UInew_obj(undo);
        }
        if (new_light_focus) {
            ImGui::SetNextWindowFocus();
            new_light_focus = false;
        }
        if (new_light_window) {
            UInew_light(scene, undo);
        }
    }
}

void Manager::UInew_light(Scene &scene, Undo &undo) {

    unsigned int idx = 0;

    ImGui::SetNextWindowSizeConstraints({200.0f, 0.0f}, {FLT_MAX,FLT_MAX});
    ImGui::Begin("New Light", &new_light_window,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    static Spectrum color = Spectrum(1.0f);
    static float intensity = 1.0f;

    ImGui::Text("Radiance");
    ImGui::ColorPicker3("Spectrum", color.data);
    ImGui::InputFloat("Intensity", &intensity);
    ImGui::Separator();

    ImGui::Text("Light Objects");

    if (ImGui::CollapsingHeader("Directional Light")) {
        ImGui::PushID(idx++);
        static Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);

        ImGui::InputFloat3("Direction", direction.data, "%.2f");

        if (ImGui::Button("Add")) {
            Scene_Light light(Light_Type::directional, scene.reserve_id(), {});
            light.opt.spectrum = color;
            light.opt.intensity = intensity;
            light.pose = Pose::rotated(Mat4::rotate_to(direction.unit()).to_euler());
            light.dirty();
            undo.add_light(std::move(light));
            new_light_window = false;
        }
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Point Light")) {
        ImGui::PushID(idx++);
        if (ImGui::Button("Add")) {
            Scene_Light light(Light_Type::point, scene.reserve_id(), {});
            light.opt.spectrum = color;
            light.opt.intensity = intensity;
            undo.add_light(std::move(light));
            new_light_window = false;
        }
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Spot Light")) {
        ImGui::PushID(idx++);
        static Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);
        static Vec2 angles = Vec2(30.0f, 35.0f);

        ImGui::InputFloat3("Direction", direction.data, "%.2f");
        ImGui::InputFloat2("Angle Cutoffs", angles.data, "%.2f");
        angles = angles.range(0.0f, 360.0f);

        if (ImGui::Button("Add")) {
            Scene_Light light(Light_Type::spot, scene.reserve_id(), {});
            light.opt.spectrum = color;
            light.opt.intensity = intensity;
            light.pose = Pose::rotated(Mat4::rotate_to(direction.unit()).to_euler());
            light.opt.angle_bounds = angles;
            light.dirty();
            undo.add_light(std::move(light));
            new_light_window = false;
        }
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Area Light (Rectangle)")) {
        ImGui::PushID(idx++);
        static Vec2 size = Vec2(1.0f);

        ImGui::InputFloat2("Size", size.data, "%.2f");
        size = clamp(size, Vec2(0.0f), size);

        if (ImGui::Button("Add")) {
            Scene_Light light(Light_Type::rectangle, scene.reserve_id(), {});
            light.opt.spectrum = color;
            light.opt.intensity = intensity;
            light.opt.size = size;
            light.dirty();
            undo.add_light(std::move(light));
            new_light_window = false;
        }
        ImGui::PopID();
    }

    if (!scene.has_env_light()) {
        ImGui::Separator();

        ImGui::Text("Environment Lights (up to one)");

        if (ImGui::CollapsingHeader("Hemisphere Light")) {
            ImGui::PushID(idx++);
            if (ImGui::Button("Add")) {
                Scene_Light light(Light_Type::hemisphere, scene.reserve_id(), {});
                light.opt.spectrum = color;
                light.opt.intensity = intensity;
                light.dirty();
                undo.add_light(std::move(light));
                new_light_window = false;
            }
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Sphere Light")) {
            ImGui::PushID(idx++);
            if (ImGui::Button("Add")) {
                Scene_Light light(Light_Type::sphere, scene.reserve_id(), {});
                light.opt.spectrum = color;
                light.opt.intensity = intensity;
                light.dirty();
                undo.add_light(std::move(light));
                new_light_window = false;
            }
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Environment Map")) {
            ImGui::PushID(idx++);
            if (ImGui::Button("Add")) {
                Scene_Light light(Light_Type::sphere, scene.reserve_id(), {});
                load_image(light);
                undo.add_light(std::move(light));
                new_light_window = false;
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}

void Manager::UInew_obj(Undo &undo) {

    unsigned int idx = 0;

    auto add_mesh = [&, this](std::string n, GL::Mesh &&mesh, bool flip = false) {
        Halfedge_Mesh hm;
        hm.from_mesh(mesh);
        if (flip)
            hm.flip();
        undo.add_obj(std::move(hm), n);
        new_obj_window = false;
    };

    ImGui::SetNextWindowSizeConstraints({200.0f, 0.0f}, {FLT_MAX,FLT_MAX});
    ImGui::Begin("New Object", &new_obj_window,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Cube")) {
        ImGui::PushID(idx++);
        static float R = 1.0f;
        ImGui::SliderFloat("Side Length", &R, 0.01f, 10.0f, "%.2f");
        if (ImGui::Button("Add")) {
            add_mesh("Cube", Util::cube_mesh(R / 2.0f), true);
        }
        ImGui::PopID();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Square")) {
        ImGui::PushID(idx++);
        static float R = 1.0f;
        ImGui::SliderFloat("Side Length", &R, 0.01f, 10.0f, "%.2f");
        if (ImGui::Button("Add")) {
            add_mesh("Square", Util::square_mesh(R / 2.0f));
        }
        ImGui::PopID();
    }

#if 0 // These have some problems converting to halfedge
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Cylinder")) {
        ImGui::PushID(idx++);
        static float R = 0.5f, H = 2.0f;
        static int S = 12;
        ImGui::SliderFloat("Radius", &R, 0.01f, 10.0f, "%.2f");
        ImGui::SliderFloat("Height", &H, 0.01f, 10.0f, "%.2f");
        ImGui::SliderInt("Sides", &S, 3, 100);
        if (ImGui::Button("Add")) {
            add_mesh("Cylinder", Util::cyl_mesh(R, H, S));
        }
        ImGui::PopID();
    }

    ImGui::Separator();

	if(ImGui::CollapsingHeader("Torus")) {
		ImGui::PushID(idx++);
		static float IR = 0.8f, OR = 1.0f;
		static int SEG = 32, S = 16;
		ImGui::SliderFloat("Inner Radius", &IR, 0.01f, 10.0f, "%.2f");
		ImGui::SliderFloat("Outer Radius", &OR, 0.01f, 10.0f, "%.2f");
		ImGui::SliderInt("Segments", &SEG, 3, 100);
		ImGui::SliderInt("Sides", &S, 3, 100);
		if(ImGui::Button("Add")) {
			add_mesh("Torus", Util::torus_mesh(IR, OR, SEG, S));
		}
		ImGui::PopID();
	}

	ImGui::Separator();

    if (ImGui::CollapsingHeader("Cone")) {
        ImGui::PushID(idx++);
        static float BR = 1.0f, TR = 0.1f, H = 1.0f;
        static int S = 12;
        ImGui::SliderFloat("Bottom Radius", &BR, 0.01f, 10.0f, "%.2f");
        ImGui::SliderFloat("Top Radius", &TR, 0.01f, 10.0f, "%.2f");
        ImGui::SliderFloat("Height", &H, 0.01f, 10.0f, "%.2f");
        ImGui::SliderInt("Sides", &S, 3, 100);
        if (ImGui::Button("Add")) {
            add_mesh("Cone", Util::cone_mesh(BR, TR, H, S));
        }
        ImGui::PopID();
    }
#endif

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Sphere")) {
        ImGui::PushID(idx++);
        static float R = 1.0f;
        ImGui::SliderFloat("Radius", &R, 0.01f, 10.0f, "%.2f");
        if (ImGui::Button("Add")) {
            Scene_Object &obj = undo.add_obj(GL::Mesh(), "Sphere");
            obj.opt.shape_type = PT::Shape_Type::sphere;
            obj.opt.shape = PT::Shape(PT::Sphere(R));
            obj.set_mesh_dirty();
            new_obj_window = false;
        }
        ImGui::PopID();
    }
    ImGui::End();
}

void Manager::set_error(std::string msg) {
    if (msg.empty())
        return;
    error_msg = msg;
    error_shown = true;
}

void Manager::render_ui(Scene &scene, Undo &undo, Camera &cam) {

    float height = UImenu(scene, undo);
    UIsidebar(scene, undo, height, cam);
    UIerror();
    UIstudent();
    if (settings_shown)
        UIsettings();

    set_error(animate.pump_output(scene));
}

Rig &Manager::get_rig() { return rig; }

Render &Manager::get_render() { return render; }

Animate &Manager::get_animate() { return animate; }

const Settings &Manager::get_settings() const { return settings; }

void Manager::UIsettings() {

    ImGui::Begin("Preferences", &settings_shown,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

    ImGui::InputInt("Multisampling", &settings.samples);

    int max = GL::max_msaa();
    if (settings.samples < 1)
        settings.samples = 1;
    if (settings.samples > max)
        settings.samples = max;

    if (ImGui::Button("Apply")) {
        Renderer::get().set_samples(settings.samples);
    }

    ImGui::Separator();
    ImGui::Text("GPU: %s", GL::renderer().c_str());
    ImGui::Text("OpenGL: %s", GL::version().c_str());

    ImGui::End();
}

void Manager::UIstudent() {
    if (!debug_shown)
        return;
    ImGui::Begin("Debug Data", &debug_shown, ImGuiWindowFlags_NoSavedSettings);
#ifndef SCOTTY3D_BUILD_REF
    student_debug_ui();
#endif
    ImGui::End();
}

void Manager::UIerror() {
    if (!error_shown)
        return;
    Vec2 center = window_dim / 2.0f;
    ImGui::SetNextWindowPos(Vec2{center.x, center.y}, 0, Vec2{0.5f, 0.5f});
    ImGui::Begin("Errors", &error_shown,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);
    if (!error_msg.empty())
        ImGui::Text("%s", error_msg.c_str());
    if (ImGui::Button("Close")) {
        error_shown = false;
    }
    ImGui::End();
}

float Manager::UImenu(Scene &scene, Undo &undo) {

    auto mode_button = [this](Gui::Mode m, std::string name) -> bool {
        bool active = m == mode;
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        bool clicked = ImGui::Button(name.c_str());
        if (active)
            ImGui::PopStyleColor();
        return clicked;
    };

    float menu_height = 0.0f;
    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem("Open Scene (Ctrl+o)"))
                load_scene(scene, undo, true);
            if (ImGui::MenuItem("Export Scene (Ctrl+e)"))
                write_scene(scene);
            if (ImGui::MenuItem("Save Scene (Ctrl+s)"))
                save_scene(scene);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {

            if (ImGui::MenuItem("Undo (Ctrl+z)"))
                undo.undo();
            if (ImGui::MenuItem("Redo (Ctrl+y)"))
                undo.redo();
            if (ImGui::MenuItem("Edit Debug Data (Ctrl+d)"))
                debug_shown = true;
            if (ImGui::MenuItem("Scotty3D Settings"))
                settings_shown = true;
            ImGui::EndMenu();
        }

        if (mode_button(Gui::Mode::layout, "Layout")) {
            mode = Gui::Mode::layout;
            if (widgets.active == Widget_Type::bevel)
                widgets.active = Widget_Type::move;
        }

        if (mode_button(Gui::Mode::model, "Model"))
            mode = Gui::Mode::model;

        if (mode_button(Gui::Mode::render, "Render"))
            mode = Gui::Mode::render;

        if (mode_button(Gui::Mode::rig, "Rig"))
            mode = Gui::Mode::rig;

        if (mode_button(Gui::Mode::animate, "Animate"))
            mode = Gui::Mode::animate;

        if (mode_button(Gui::Mode::simulate, "Simulate"))
            mode = Gui::Mode::simulate;

        ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);

        menu_height = ImGui::GetWindowSize().y;
        ImGui::EndMainMenuBar();
    }

    return menu_height;
}

void Manager::create_baseplane() {

    const int R = 25;
    for (int i = -R; i <= R; i++) {
        if (i == 0) {
            baseplane.add(Vec3{-R, 0, i}, Vec3{R, 0, i}, Color::red);
            baseplane.add(Vec3{i, 0, -R}, Vec3{i, 0, R}, Color::blue);
            continue;
        }
        baseplane.add(Vec3{i, 0, -R}, Vec3{i, 0, R}, Color::baseplane);
        baseplane.add(Vec3{-R, 0, i}, Vec3{R, 0, i}, Color::baseplane);
    }
}

void Manager::end_drag(Undo &undo, Scene &scene) {

    if (!widgets.is_dragging())
        return;

    Scene_Item &obj = *scene.get(layout.selected());

    switch (mode) {
    case Mode::animate: {
        animate.end_transform(undo, obj);
    } break;

    case Mode::render:
    case Mode::layout: {
        layout.end_transform(undo, obj);
    } break;
    case Mode::model: {
        if (obj.is<Scene_Object>()) {
            std::string err = model.end_transform(widgets, undo, obj.get<Scene_Object>());
            set_error(err);
        }
    } break;
    case Mode::rig: {
        if (obj.is<Scene_Object>()) {
            rig.end_transform(widgets, undo, obj.get<Scene_Object>());
        }
    } break;
    default:
        break;
    }

    widgets.end_drag();
}

void Manager::drag_to(Scene &scene, Vec3 cam, Vec2 spos, Vec3 dir) {

    if (!widgets.is_dragging())
        return;
    Scene_Item &obj = *scene.get(layout.selected());

    Vec3 pos;
    switch (mode) {
    case Mode::animate: {
        pos = animate.selected_pos(obj);
    } break;
    case Mode::render:
    case Mode::layout: {
        pos = layout.selected_pos(scene);
    } break;
    case Mode::model: {
        pos = model.selected_pos();
    } break;
    case Mode::rig: {
        pos = rig.selected_pos();
    } break;
    default:
        break;
    }

    widgets.drag_to(pos, cam, spos, dir, mode == Mode::model);

    switch (mode) {
    case Mode::animate: {
        animate.apply_transform(widgets, obj);
    } break;
    case Mode::render:
    case Mode::layout: {
        layout.apply_transform(obj, widgets);
    } break;
    case Mode::model: {
        model.apply_transform(widgets);
    } break;
    case Mode::rig: {
        rig.apply_transform(widgets);
    } break;
    default:
        break;
    }
}

bool Manager::select(Scene &scene, Undo &undo, Scene_ID id, Vec3 cam, Vec2 spos, Vec3 dir) {

    widgets.select(id);

    switch (mode) {
    case Mode::render:
    case Mode::layout: {
        layout.select(scene, widgets, id, cam, spos, dir);
    } break;

    case Mode::model: {
        std::string err = model.select(widgets, id, cam, spos, dir);
        set_error(err);
    } break;

    case Mode::rig: {
        rig.select(scene, widgets, undo, id, cam, spos, dir);
    } break;

    case Mode::animate: {
        if (animate.select(scene, widgets, layout.selected(), id, cam, spos, dir))
            layout.set_selected(id);
    } break;

    default:
        break;
    }

    return widgets.is_dragging();
}

void Manager::set_select(Scene_ID id) {
    clear_select();
    layout.set_selected(id);
}

void Manager::clear_select() {
    switch (mode) {
    case Mode::render:
    case Mode::animate:
    case Mode::layout:
        layout.clear_select();
        break;
    case Mode::rig:
        rig.clear_select();
        break;
    case Mode::model:
        model.clear_select();
        break;
    default:
        break;
    }
}

void Manager::render_3d(Scene &scene, Camera &camera) {

    Mat4 view = camera.get_view();

    animate.update(scene);

    if (mode == Mode::layout || mode == Mode::render || mode == Mode::animate) {
        scene.for_items([&, this](Scene_Item &item) {
            bool render = item.id() != layout.selected();
            if (item.is<Scene_Light>()) {
                const Scene_Light &light = item.get<Scene_Light>();
                if (light.opt.type == Light_Type::sphere ||
                    light.opt.type == Light_Type::hemisphere)
                    render = true;
            }

            if (render) {
                item.render(view);
            }
        });
    }

    Renderer::get().lines(baseplane, view, Mat4::I, 1.0f);

    auto selected = scene.get(layout.selected());
    switch (mode) {
    case Mode::layout: {
        layout.render(selected, widgets, camera);
    } break;

    case Mode::model: {
        model.render(selected, widgets, camera);
    } break;

    case Mode::render: {
        render.render(selected, widgets, camera);
    } break;

    case Mode::rig: {
        rig.render(selected, widgets, camera);
    } break;

    case Mode::animate: {
        animate.render(scene, selected, widgets, camera);
    } break;

    default:
        break;
    }
}

void Manager::hover(Vec2 pixel, Vec3 cam, Vec2 spos, Vec3 dir) {
    if (mode == Mode::model) {
        model.set_hover(Renderer::get().read_id(pixel));
    } else if (mode == Mode::rig) {
        rig.hover(cam, spos, dir);
    }
}

} // namespace Gui
