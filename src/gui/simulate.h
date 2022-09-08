
#pragma once

#include "../pathtracer/pathtracer.h"
#include "../scene/particles.h"
#include "../util/thread_pool.h"
#include "../util/timer.h"

#include "widgets.h"
#include <SDL.h>

namespace Gui {

enum class Mode : uint8_t;
class Manager;

class Simulate {
public:
	Simulate();
	~Simulate();

	void update(Scene& scene, Undo& undo);
	void pause();
	void resume();

	void step(Scene& scene, float dt);

	void clear_particles(Scene& scene);
	void update_bvh(Scene& scene, Undo& undo);
	void build_scene(Scene& scene);

	void ui_sidebar(Manager& manager, Scene& scene, Undo& undo, Widgets& widgets);

private:
	PT::Aggregate scene_obj;
    std::unordered_map<std::string, Shape> shapes;
    std::unordered_map<std::string, PT::Tri_Mesh> meshes;
    
	bool use_bvh = true;

	Thread_Pool thread_pool;
	size_t cur_actions = 0;
	Timer sim_timer;
};

} // namespace Gui
