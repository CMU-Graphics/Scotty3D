
#pragma once

#include <SDL.h>
#include <mutex>

#include "../platform/gl.h"
#include "../scene/scene.h"
#include "../util/viewer.h"
#include "widgets.h"

struct Launch_Settings;
class Undo;

namespace Gui {

class Manager;

class Render {
public:
	Render() = default;

	std::pair<float, float> completion_time();
	
    void ui_sidebar(Manager& manager, Undo& undo, Scene& scene, View_3D& user_cam);
    void render(View_3D& user_cam);

private:
	GL::Lines bvh_viz, bvh_active;
	Widget_Render ui_render;

	// GUI Data
	bool render_ray_log = false;
	bool visualize_bvh = false;
	uint32_t bvh_level = 0;
	uint32_t bvh_levels = 0;
};

} // namespace Gui
