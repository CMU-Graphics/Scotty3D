
#pragma once

#include "../lib/mathlib.h"
#include "../util/hdr_image.h"
#include "../util/thread_pool.h"
#include "../util/timer.h"
#include "../rays/object.h"

#include "scene.h"

// NOTE :
// these ideas are largely based off of pathtracer.h minus pathtracer-specific components
// this header idea considers the possibility of 3 implementations of this renderer interface:
//		hardware rasterizer, software rasterizer, and pathtracer
// (currently hardware rasterizer and pathtracer do not share the interface, and the render widget
// just runs different code based on the selected option)

namespace Gui {
class Widget_Render;
}

class Renderer {
public:
	// renderer constructors and stuff
	Renderer();
	Renderer(Vec2 screen_dim);
	Renderer(Gui::Widget_Render& gui, Vec2 screen_dim);
	~Renderer();

	// methods to set render parameters
	void set_params(uint32_t w, uint32_t h, uint32_t pixel_samples);
	void set_samples(uint32_t samples);
	void set_camera(Camera cam);

	// method to build some representation of the scene
	void build_scene(Scene& scene);

	// methods related to running render
	void begin_render(Scene& scene, const Camera& camera);
	void cancel();
	bool in_progress() const;
	float progress() const;
	std::pair<float, float> completion_time() const;

	// get the resulting image back
	const HDR_Image& get_output();

private:
	Gui::Widget_Render* gui;
	Thread_Pool thread_pool;
	bool cancel_flag = false;
	Timer render_timer, build_timer;

	PT::Object scene;

	Camera camera;
	uint32_t out_w, out_h, n_samples;
};