#pragma once

/*
 * `Rasterizer` renders a `Scene` using the software rasterization pipeline (pipeline.h).
 *
 * It's a thin wrapper around a RasterJob, which holds a copy of the necessary Scene
 * data to run asynchronously.
 *
 */

#include <future>
#include <string>

#include "../util/hdr_image.h"

class Scene;
struct RasterJob;
struct Framebuffer;
namespace Instance {
class Camera;
};

class Rasterizer {
public:
	using Render_Report = std::pair<float, HDR_Image>;
	// TODO: should probably move to something like this:
	/*
	struct Render_Report {
	    std::string description; //description of current task
	    float progress; //in [0,1], a rough estimate
	    HDR_Image image; //current image (via Framebufer::resolve())
	};
	*/

	// to start rendering a scene, construct a Rasterizer and pass the scene and camera through
	// which to render it.
	//
	// relevant data from scene and camera will be copied (you can delete or modify them during the
	// render) camera does not need to be member of the scene report_fn will be called with updates
	// on progress and copies of the image produced so far.
	// 		(report_fn will run in a separate thread! be careful to synchronize.)
	Rasterizer(Scene const& scene, Instance::Camera const& camera,
	           std::function<void(Render_Report)>&& report_fn);

	// Destroying the rasterizer will cancel rasterization:
	~Rasterizer();

	// You can also cancel it directly:
	void cancel();

	// ...or signal it to quit eventually:
	void signal();

	// ...or wait for it to finish:
	void wait();

	// ...or just ask if it is still running:
	bool in_progress();

	// if finished, you may read:
	// 		(otherwise, beware that async job may be writing these)
	float completion_time = std::numeric_limits<float>::quiet_NaN();
	Framebuffer const* framebuffer; // points into the RasterJob

	// since 'Rasterizer' represents a unique running rasterization thread, you can't copy it:
	Rasterizer(Rasterizer const&) = delete;

private:
	// contains all the state needed by the other thread:
	std::unique_ptr<RasterJob> job;

	// future used to manage async compute thread:
	std::future<void> future;
};
