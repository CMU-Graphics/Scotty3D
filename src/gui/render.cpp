
#include <imgui/imgui_custom.h>
#include <nfd/nfd.h>
#include <sf_libs/stb_image_write.h>

#include "../app.h"
#include "../platform/platform.h"
#include "../platform/renderer.h"

#include "manager.h"
#include "render.h"

namespace Gui {

void Render::render(View_3D& user_cam) {

    auto V = user_cam.get_view();

    if(visualize_bvh) {
        Renderer::get().lines(bvh_viz, V);
        Renderer::get().lines(bvh_active, V);
    }
    if(render_ray_log) {
        ui_render.render_log(V);
    }
}

void Render::ui_sidebar(Manager& manager, Undo& undo, Scene& scene, View_3D& user_cam) {

    using namespace ImGui;

    Separator();
	Text("Visualize");

	Checkbox("Draw BVH", &visualize_bvh);
	Checkbox("Draw rays", &render_ray_log);

	bool update_bvh = false;

	if (visualize_bvh) {
		if (SliderUInt32("Level", &bvh_level, 0, bvh_levels)) {
			update_bvh = true;
		}
	}
	bvh_level = std::min(bvh_level, bvh_levels);

	std::string err;
	update_bvh = update_bvh || ui_render.ui_render(scene, manager, undo, user_cam, err);
	manager.set_error(err);

	if (update_bvh) {
		update_bvh = false;
		bvh_viz.clear();
		bvh_active.clear();
		bvh_levels = ui_render.tracer().visualize_bvh(bvh_viz, bvh_active, bvh_level);
	}

	if (Button("Open Render Window")) {
		ui_render.open(scene);
	}
}

std::pair<float, float> Render::completion_time() {
	return ui_render.tracer().completion_time();
}

} // namespace Gui
