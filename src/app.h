
#pragma once

#include <SDL.h>
#include <map>
#include <string>

#include "gui/manager.h"
#include "lib/mathlib.h"
#include "util/viewer.h"

#include "scene/animator.h"
#include "scene/scene.h"
#include "scene/undo.h"

class Platform;

struct Launch_Settings {
	std::string scene_file;
};

class App {
public:
	App(Launch_Settings set, Platform* plt = nullptr);
	~App();

	void render();
	bool quit();
	void event(SDL_Event e);

private:
	void apply_window_dim(Vec2 new_dim);
	Vec3 screen_to_world(Vec2 mouse);

	// Camera data
	enum class View_Control : uint8_t { none, orbit, move };
	Vec2 window_dim, mouse_press;
	bool selection_changed = false;
	View_Control cam_mode = View_Control::none;
	View_3D gui_camera;
	Mat4 view, proj, iviewproj;

	// Systems
	Undo undo;
	Scene scene;
	Animator animator;
	Gui::Manager gui;
	Platform* plt = nullptr;

	bool gui_capture = false;
};
