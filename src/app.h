
#pragma once

#include <SDL2/SDL.h>
#include <map>
#include <string>

#include "gui/manager.h"
#include "lib/mathlib.h"
#include "util/camera.h"

#include "scene/scene.h"
#include "scene/undo.h"

class Platform;

class App {
public:
    struct Settings {

        std::string scene_file;
        std::string env_map_file;
        bool headless = false;

        // If headless is true, use all of these
        std::string output_file = "out.png";
        int w = 640;
        int h = 360;
        int s = 128;
        int ls = 16;
        int d = 4;
        bool animate = false;
        float exp = 1.0f;
        bool w_from_ar = false;
    };

    App(Settings set, Platform* plt = nullptr);
    ~App();

    void render();
    bool quit();
    void event(SDL_Event e);

private:
    void apply_window_dim(Vec2 new_dim);
    Vec3 screen_to_world(Vec2 mouse);

    // Camera data
    enum class Camera_Control { none, orbit, move };
    Vec2 window_dim, mouse_press;
    bool selection_changed = false;
    Camera_Control cam_mode = Camera_Control::none;
    Camera camera;
    Mat4 view, proj, iviewproj;

    // Systems
    Platform* plt = nullptr;
    Scene scene;
    Gui::Manager gui;
    Undo undo;

    bool gui_capture = false;
};
