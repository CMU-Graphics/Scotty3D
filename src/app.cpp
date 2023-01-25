#include <fstream>

#include <SDL.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>

#include "app.h"
#include "geometry/util.h"
#include "platform/platform.h"
#include "platform/renderer.h"
#include "test.h"
#include "scene/io.h"

App::App(Launch_Settings set, Platform* plt)
	: window_dim(plt ? plt->window_draw() : Vec2{1.0f}),
	  gui_camera(plt ? plt->window_draw() : Vec2{1.0f}), undo(scene, animator, gui),
	  gui(scene, undo, animator, window_dim), plt(plt) {

	if (!set.scene_file.empty()) {
		info("Loading scene file...");
		gui.load_scene(&set.scene_file, Gui::Manager::Load::new_scene);
	}

	GL::global_params();
	Renderer::setup(window_dim);
	apply_window_dim(plt->window_draw());
}

App::~App() {
	gui.shutdown();
	Renderer::shutdown();
}

bool App::quit() {
	return gui.quit();
}

void App::event(SDL_Event e) {

	ImGuiIO& IO = ImGui::GetIO();
	IO.DisplayFramebufferScale = plt->scale(Vec2{1.0f, 1.0f});

	Gui::Modifiers mods = 0;

	if (plt->is_down(SDL_SCANCODE_LSHIFT) || plt->is_down(SDL_SCANCODE_RSHIFT)) {
		mods |= Gui::AppendBit;
	}
	if (plt->is_down(SDL_SCANCODE_LCTRL) || plt->is_down(SDL_SCANCODE_RCTRL)) {
		mods |= Gui::SnapBit;
	}


	switch (e.type) {
	case SDL_KEYDOWN: {
		if (IO.WantCaptureKeyboard) break;
		if (gui.keydown(e.key.keysym, gui_camera)) break;

#ifdef __APPLE__
		Uint16 mod = KMOD_GUI;
#else
		Uint16 mod = KMOD_CTRL;
#endif

		if (e.key.keysym.sym == SDLK_z) {
			if (e.key.keysym.mod & mod) {
				undo.undo();
			}
		} else if (e.key.keysym.sym == SDLK_y) {
			if (e.key.keysym.mod & mod) {
				undo.redo();
			}
		}
	} break;

	case SDL_WINDOWEVENT: {
		if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
		    e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {

			apply_window_dim(plt->window_draw());
		}
	} break;

	case SDL_MOUSEMOTION: {

		Vec2 d(e.motion.xrel, e.motion.yrel);
		Vec2 p = plt->scale(Vec2{e.button.x, e.button.y});
		Vec2 dim = plt->window_draw();
		Vec2 n = Vec2(2.0f * p.x / dim.x - 1.0f, 2.0f * p.y / dim.y - 1.0f);

		if (gui_capture) {
			gui.drag_to(gui_camera.pos(), n, screen_to_world(p), mods);
		} else if (cam_mode == View_Control::orbit) {
			gui_camera.mouse_orbit(d);
		} else if (cam_mode == View_Control::move) {
			gui_camera.mouse_move(d);
		} else {
			uint32_t id = Renderer::get().read_id(p);
			gui.hover(id, gui_camera.pos(), n, screen_to_world(p), mods);
		}

	} break;

	case SDL_MOUSEBUTTONDOWN: {

		if (IO.WantCaptureMouse) break;

		Vec2 p = plt->scale(Vec2{e.button.x, e.button.y});
		Vec2 dim = plt->window_draw();
		Vec2 n = Vec2(2.0f * p.x / dim.x - 1.0f, 2.0f * p.y / dim.y - 1.0f);

		if (e.button.button == SDL_BUTTON_LEFT) {

			uint32_t id = Renderer::get().read_id(p);

			if (cam_mode == View_Control::none
			 && (plt->is_down(SDL_SCANCODE_LALT) || plt->is_down(SDL_SCANCODE_RALT))) {

				cam_mode = View_Control::orbit;
			} else if (gui.select(id, gui_camera.pos(), n, screen_to_world(p), mods)) {
				cam_mode = View_Control::none;
				plt->grab_mouse();
				gui_capture = true;
			} else if (id) {
				selection_changed = true;
			}

			mouse_press = Vec2(e.button.x, e.button.y);

		} else if (e.button.button == SDL_BUTTON_RIGHT) {
			if (cam_mode == View_Control::none) {
				cam_mode = View_Control::move;
			}
		} else if (e.button.button == SDL_BUTTON_MIDDLE) {
			if (cam_mode == View_Control::none &&
			    ((plt->is_down(SDL_SCANCODE_LALT) || plt->is_down(SDL_SCANCODE_RALT)))) {
				cam_mode = View_Control::move;
			} else {
				cam_mode = View_Control::orbit;
			}
		}

	} break;

	case SDL_MOUSEBUTTONUP: {

		Vec2 p = plt->scale(Vec2{e.button.x, e.button.y});
		Vec2 dim = plt->window_draw();
		Vec2 n = Vec2(2.0f * p.x / dim.x - 1.0f, 2.0f * p.y / dim.y - 1.0f);

		if (e.button.button == SDL_BUTTON_LEFT) {
			if (!IO.WantCaptureMouse && gui_capture) {
				gui_capture = false;
				gui.drag_to(gui_camera.pos(), n, screen_to_world(p), mods);
				gui.end_drag();
				plt->ungrab_mouse();
				break;
			} else {
				Vec2 diff = mouse_press - Vec2(e.button.x, e.button.y);
				if (!selection_changed && diff.norm() <= 3) {
					gui.clear_select();
				}
				selection_changed = false;
			}
		}

		if ((e.button.button == SDL_BUTTON_LEFT && cam_mode == View_Control::orbit) ||
		    (e.button.button == SDL_BUTTON_MIDDLE && cam_mode == View_Control::orbit) ||
		    (e.button.button == SDL_BUTTON_RIGHT && cam_mode == View_Control::move) ||
		    (e.button.button == SDL_BUTTON_MIDDLE && cam_mode == View_Control::move)) {
			cam_mode = View_Control::none;
		}

	} break;

	case SDL_MOUSEWHEEL: {
		if (IO.WantCaptureMouse) break;
		gui_camera.mouse_radius(static_cast<float>(e.wheel.y));
	} break;
	}
}

void App::render() {

	proj = gui_camera.get_proj();
	view = gui_camera.get_view();
	iviewproj = (proj * view).inverse();

	Renderer& r = Renderer::get();
	r.begin();
	r.proj(proj);

	gui.render_3d(gui_camera);

	r.complete();

	gui.render_ui(gui_camera);
}

Vec3 App::screen_to_world(Vec2 mouse) {

	Vec2 t(2.0f * mouse.x / window_dim.x - 1.0f, 1.0f - 2.0f * mouse.y / window_dim.y);
	Vec3 p = iviewproj * Vec3(t.x, t.y, 0.1f);
	return (p - gui_camera.pos()).unit();
}

void App::apply_window_dim(Vec2 new_dim) {
	window_dim = new_dim;
	gui_camera.set_ar(window_dim);
	gui.update_dim(plt->window_size());
	Renderer::get().update_dim(window_dim);
}
