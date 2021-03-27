
#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>

#include "app.h"
#include "geometry/util.h"
#include "platform/platform.h"
#include "scene/renderer.h"

App::App(Settings set, Platform* plt)
    : window_dim(plt ? plt->window_draw() : Vec2{1.0f}),
      camera(plt ? plt->window_draw() : Vec2{1.0f}), plt(plt), scene(Gui::n_Widget_IDs),
      gui(scene, plt ? plt->window_size() : Vec2{1.0f}), undo(scene, gui) {

    if(!set.headless) assert(plt);

    std::string err;
    bool loaded_scene = true;

    if(!set.scene_file.empty()) {
        info("Loading scene file...");
        Scene::Load_Opts opts;
        opts.new_scene = true;
        err = scene.load(opts, undo, gui, set.scene_file);
        gui.set_file(set.scene_file);
    }

    if(!err.empty()) {
        warn("Error loading scene: %s", err.c_str());
        loaded_scene = false;
    }

    if(!set.env_map_file.empty()) {
        info("Loading environment map...");
        err = scene.set_env_map(set.env_map_file);
        if(!err.empty()) warn("Error loading environment map: %s", err.c_str());
    }

    if(!set.headless) {
        GL::global_params();
        Renderer::setup(window_dim);
        apply_window_dim(plt->window_draw());
    } else if(loaded_scene) {

        info("Rendering scene...");
        err = gui.get_render().headless_render(gui.get_animate(), scene, set.output_file,
                                               set.animate, set.w, set.h, set.s, set.ls, set.d,
                                               set.exp, set.w_from_ar);

        if(!err.empty())
            warn("Error rendering scene: %s", err.c_str());
        else {
            auto [build, render] = gui.get_render().completion_time();
            info("Built scene in %.2fs, rendered in %.2fs", build, render);
        }
    }
}

App::~App() {
    Renderer::shutdown();
}

bool App::quit() {
    return gui.quit(undo);
}

void App::event(SDL_Event e) {

    ImGuiIO& IO = ImGui::GetIO();
    IO.DisplayFramebufferScale = plt->scale(Vec2{1.0f, 1.0f});

    switch(e.type) {
    case SDL_KEYDOWN: {
        if(IO.WantCaptureKeyboard) break;
        if(gui.keydown(undo, e.key.keysym, scene, camera)) break;

#ifdef __APPLE__
        Uint16 mod = KMOD_GUI;
#else
        Uint16 mod = KMOD_CTRL;
#endif

        if(e.key.keysym.sym == SDLK_z) {
            if(e.key.keysym.mod & mod) {
                undo.undo();
            }
        } else if(e.key.keysym.sym == SDLK_y) {
            if(e.key.keysym.mod & mod) {
                undo.redo();
            }
        }
    } break;

    case SDL_WINDOWEVENT: {
        if(e.window.event == SDL_WINDOWEVENT_RESIZED ||
           e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {

            apply_window_dim(plt->window_draw());
        }
    } break;

    case SDL_MOUSEMOTION: {

        Vec2 d(e.motion.xrel, e.motion.yrel);
        Vec2 p = plt->scale(Vec2{e.button.x, e.button.y});
        Vec2 dim = plt->window_draw();
        Vec2 n = Vec2(2.0f * p.x / dim.x - 1.0f, 2.0f * p.y / dim.y - 1.0f);

        if(gui_capture) {
            gui.drag_to(scene, camera.pos(), n, screen_to_world(p));
        } else if(cam_mode == Camera_Control::orbit) {
            camera.mouse_orbit(d);
        } else if(cam_mode == Camera_Control::move) {
            camera.mouse_move(d);
        } else {
            gui.hover(p, camera.pos(), n, screen_to_world(p));
        }

    } break;

    case SDL_MOUSEBUTTONDOWN: {

        if(IO.WantCaptureMouse) break;

        Vec2 p = plt->scale(Vec2{e.button.x, e.button.y});
        Vec2 dim = plt->window_draw();
        Vec2 n = Vec2(2.0f * p.x / dim.x - 1.0f, 2.0f * p.y / dim.y - 1.0f);

        if(e.button.button == SDL_BUTTON_LEFT) {

            Scene_ID id = Renderer::get().read_id(p);

            if(cam_mode == Camera_Control::none &&
               (plt->is_down(SDL_SCANCODE_LSHIFT) | plt->is_down(SDL_SCANCODE_RSHIFT))) {
                cam_mode = Camera_Control::orbit;
            } else if(gui.select(scene, undo, id, camera.pos(), n, screen_to_world(p))) {
                cam_mode = Camera_Control::none;
                plt->grab_mouse();
                gui_capture = true;
            } else if(id) {
                selection_changed = true;
            }

            mouse_press = Vec2(e.button.x, e.button.y);

        } else if(e.button.button == SDL_BUTTON_RIGHT) {
            if(cam_mode == Camera_Control::none) {
                cam_mode = Camera_Control::move;
            }
        } else if(e.button.button == SDL_BUTTON_MIDDLE) {
            cam_mode = Camera_Control::orbit;
        }

    } break;

    case SDL_MOUSEBUTTONUP: {

        Vec2 p = plt->scale(Vec2{e.button.x, e.button.y});
        Vec2 dim = plt->window_draw();
        Vec2 n = Vec2(2.0f * p.x / dim.x - 1.0f, 2.0f * p.y / dim.y - 1.0f);

        if(e.button.button == SDL_BUTTON_LEFT) {
            if(!IO.WantCaptureMouse && gui_capture) {
                gui_capture = false;
                gui.drag_to(scene, camera.pos(), n, screen_to_world(p));
                gui.end_drag(undo, scene);
                plt->ungrab_mouse();
                break;
            } else {
                Vec2 diff = mouse_press - Vec2(e.button.x, e.button.y);
                if(!selection_changed && diff.norm() <= 3) {
                    gui.clear_select();
                }
                selection_changed = false;
            }
        }

        if((e.button.button == SDL_BUTTON_LEFT && cam_mode == Camera_Control::orbit) ||
           (e.button.button == SDL_BUTTON_MIDDLE && cam_mode == Camera_Control::orbit) ||
           (e.button.button == SDL_BUTTON_RIGHT && cam_mode == Camera_Control::move)) {
            cam_mode = Camera_Control::none;
        }

    } break;

    case SDL_MOUSEWHEEL: {
        if(IO.WantCaptureMouse) break;
        camera.mouse_radius((float)e.wheel.y);
    } break;
    }
}

void App::render() {

    proj = camera.get_proj();
    view = camera.get_view();
    iviewproj = (proj * view).inverse();

    Renderer& r = Renderer::get();
    r.begin();
    r.proj(proj);

    gui.render_3d(scene, undo, camera);

    r.complete();

    gui.render_ui(scene, undo, camera);
}

Vec3 App::screen_to_world(Vec2 mouse) {

    Vec2 t(2.0f * mouse.x / window_dim.x - 1.0f, 1.0f - 2.0f * mouse.y / window_dim.y);
    Vec3 p = iviewproj * Vec3(t.x, t.y, 0.1f);
    return (p - camera.pos()).unit();
}

void App::apply_window_dim(Vec2 new_dim) {
    window_dim = new_dim;
    camera.set_ar(window_dim);
    gui.update_dim(plt->window_size());
    Renderer::get().update_dim(window_dim);
}
