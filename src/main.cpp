
#include <sf_libs/CLI11.hpp>
#include <sf_libs/stb_image_write.h>

#include "platform/platform.h"
#include "util/rand.h"
#include "lib/log.h"

#include "pathtracer/pathtracer.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/sample_pattern.h"

#include "test.h"

int main(int argc, char** argv) {

	Platform::init_console();

	//app settings:
	Launch_Settings set;

	//settings used in this file for headless operation:
	std::string tests_prefix = "";

	bool pathtrace = false;
	bool rasterize = false;

	std::string output_file = "out.png";

	std::string camera_name;
	bool animate = false;
	float exp = 1.0f;
	bool no_bvh = false;

	uint32_t film_width = -1U; //override film width (if not -1U)
	uint32_t film_height = -1U; //override film height (if not -1U)
	uint32_t film_samples = -1U; //override film samples (if not -1U)
	uint32_t film_max_ray_depth = -1U; //override film max ray depth (if not -1U)
	std::string film_sample_pattern = ""; //override film sample pattern (if not "")


	CLI::App args{"Scotty3D - Student Version"};

	auto tests_option = args.add_option("--run-tests", tests_prefix, "Run all tests starting with prefix", true);
	tests_option->expected(0, 1);

	args.add_option("-s,--scene", set.scene_file, "Scene file to load");
	args.add_flag("--trace", pathtrace, "Path trace scene without opening the GUI");
	args.add_flag("--rasterize", rasterize, "Rasterize scene without opening the GUI");
	args.add_option("-c,--camera", camera_name, "Camera instance to render (if headless)");
	args.add_option("-o,--output", output_file, "Image file to write (if headless)");
	args.add_flag("--animate", animate, "Output animation frames (if headless)");
	args.add_flag("--no_bvh", no_bvh, "Don't use BVH (if headless)");
	args.add_option("--exposure", exp, "Output exposure (if headless)");
	args.add_option("--film-width",          film_width, "Override camera film width (pixels)");
	args.add_option("--film-height",         film_height, "Override camera film height (pixels)");
	args.add_option("--film-samples",        film_samples, "Override film samples-per-pixel (for pathtracer)");
	args.add_option("--film-max-ray-depth",  film_max_ray_depth, "Override film max ray depth (for pathtracer)");
	args.add_option("--film-sample-pattern", film_sample_pattern, "Override film sample pattern (for rasterizer)");

	CLI11_PARSE(args, argc, argv);

	// if tests are to be run, run them and return error if (some) tests fail:
	if (tests_option->count() > 0) {
		if (Test::run_tests(tests_prefix)) {
			return 0;
		} else {
			return 1;
		}
	}

	//if headless render requested, do that and return:
	if (pathtrace || rasterize) {
		if (set.scene_file == "") std::cout << "ERROR: must specify a scene file via --scene when doing --trace or --rasterize." << std::endl;
		Scene scene;
		Animator animator;

		//load scene:
		try {
			std::ifstream file(set.scene_file, std::ios::binary);
			scene = Scene::load(file);
			animator = Animator::load(file);
		} catch (std::exception const &e) {
			warn("ERROR: Failed to load scene '%s': %s", set.scene_file.c_str(), e.what());
			return 1;
		}

		//find camera:
		std::weak_ptr< Instance::Camera > camera_instance = scene.get<Instance::Camera>(camera_name);
		if (!camera_instance.lock()) {
			std::string all_cameras = "Camera instances in scene:";
			for (auto const &[name, camera] : scene.instances.cameras) {
				all_cameras += "\n    '" + name + "'";
			}
			warn("ERROR: Failed to find camera: %s", camera_name.c_str());
			info("%s", all_cameras.c_str());
			return 1;
		}
		std::shared_ptr< Camera > camera = camera_instance.lock()->camera.lock();
		assert(camera && "valid scenes always have valid data references in instances");

		//override camera parameters if requested:
		if (film_width != -1U && film_height != -1U) {
			camera->film.width = film_width;
			camera->film.height = film_height;
			camera->aspect_ratio = camera->film.width / float(camera->film.height);
			std::cout << "  Set film size to [" << camera->film.width << "x" << camera->film.height << "]." << std::endl;
		} else if (film_width != -1U) {
			camera->film.width = film_width;
			camera->film.height = uint32_t(std::round(camera->film.width / camera->aspect_ratio));
			std::cout << "  Set film size to [" << camera->film.width << "x" << camera->film.height << "] (height determined from aspect ratio)." << std::endl;
		} else if (film_height != -1U) {
			camera->film.height = film_height;
			camera->film.width = uint32_t(std::round(camera->film.height * camera->aspect_ratio));
			std::cout << "  Set film size to [" << camera->film.width << "x" << camera->film.height << "] (width determined from aspect ratio)." << std::endl;
		}

		if (film_samples != -1U) {
			camera->film.samples = film_samples;
			std::cout << "  Set film path tracer samples to " << camera->film.samples << "." << std::endl;
		}

		if (film_max_ray_depth != -1U) {
			camera->film.max_ray_depth = film_max_ray_depth;
			std::cout << "  Set film max ray depth to " << camera->film.max_ray_depth << "." << std::endl;
		}

		if (film_sample_pattern != "") {
			std::vector< SamplePattern > const &patterns = SamplePattern::all_patterns();
			bool found = false;
			for (auto const &p : patterns) {
				if (p.name == film_sample_pattern) {
					camera->film.sample_pattern = p.id;
					std::cout << "  Set film rasterizer sample pattern to '" << p.name << "'." << std::endl;
					found = true;
					break;
				}
			}
			if (!found) {
				std::string all_patterns = "Available Sample Patterns:";
				for (auto const &p : patterns) {
					all_patterns += "\n    '" + p.name + "'";
				}
				warn("ERROR: Failed to find sample pattern: %s", film_sample_pattern.c_str());
				info("%s", all_patterns.c_str());
				return 1;
			}
		}


		//do the render:
		info("Render settings:");
		info("\twidth: %d", camera->film.width);
		info("\theight: %d", camera->film.height);
		info("\theight: %d", camera->film.height);
		info("\texposure: %f", exp);

		if (pathtrace) {
			info("\tsamples: %d", camera->film.samples);
			info("\tmax depth: %d", camera->film.max_ray_depth);
			info("\trender threads: %u", std::thread::hardware_concurrency());
			if (no_bvh) info("\tusing object list instead of BVH");
			info("\tpathtracing...");
		} else { assert(rasterize);
			std::string name;
			if (SamplePattern const *p = SamplePattern::from_id(camera->film.sample_pattern)) {
				name = p->name;
			} else {
				name = "???"; //this *probably* will cause rasterizer to fail anyway
			}
			info("\tsample pattern: '%s' (%d)", name.c_str(), camera->film.sample_pattern);
			info("\trasterizing...");

		}

		auto print_progress = [](float f) {
			std::cout << "Progress: [";

			int32_t console = static_cast<int32_t>(Platform::console_width());
			int32_t width = std::clamp(console - 30, 0, 50);
			if (width) {
				int32_t bar = static_cast<int32_t>(width * f);
				for (int32_t i = 0; i < bar; i++) std::cout << "-";
				for (int32_t i = bar; i < width; i++) std::cout << " ";
				std::cout << "] ";
			}

			float percent = 100.0f * f;
			if (percent < 10.0f) std::cout << "0";
			std::cout << percent << "%\r";
			std::cout.flush();
		};

		std::mutex report_mut;
		float percent_done = 0.0f;
		HDR_Image display_hdr;

		auto report_callback = [&](auto&& report) {
			std::lock_guard<std::mutex> lock(report_mut);
			if (report.first > percent_done) {
				percent_done = report.first;
				display_hdr = std::move(report.second);
			}
		};

		if (pathtrace) {
			bool quit = false;
			PT::Pathtracer pathtracer;

			pathtracer.use_bvh(!no_bvh);
			pathtracer.render(scene, camera_instance.lock(), std::move(report_callback), &quit);

			while (pathtracer.in_progress()) {
				print_progress(percent_done);
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
			std::cout << std::endl;

		} else { assert(rasterize);

			Rasterizer rasterizer(scene, *camera_instance.lock(), std::move(report_callback));
			while (rasterizer.in_progress()) {
				print_progress(percent_done);
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
			std::cout << std::endl;
		}
		info("\tdone.");

		//write frame:
		if (output_file == "") {
			std::cout << "No output was requested, not writing any file." << std::endl;
		} else {
			uint32_t data_w, data_h;
			std::vector<uint8_t> data;
			display_hdr.tonemap_to(data, exp);
			data_w = display_hdr.w;
			data_h = display_hdr.h;

			stbi_flip_vertically_on_write(true);
			if (!stbi_write_png(output_file.c_str(), data_w, data_h, 4, data.data(), data_w * 4)) {
				warn("ERROR: Failed to write output to '%s'", output_file.c_str());
				return 1;
			}
			std::cout << "Wrote result to '" << output_file << "'." << std::endl;
		}


		return 0;
	}


	{ //No tests, no render => run GUI:
		Platform plt;
		App app(set, &plt);
		plt.loop(app);
	}
	return 0;
}
