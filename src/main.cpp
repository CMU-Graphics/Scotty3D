
#include <sf_libs/CLI11.hpp>
#include <sf_libs/stb_image_write.h>

#include "platform/platform.h"
#include "util/rand.h"
#include "lib/log.h"

#include "pathtracer/pathtracer.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/sample_pattern.h"
#include "scene/io.h"

#include "test.h"

#include <filesystem>

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
	int32_t min_frame = 0;
	int32_t max_frame = -1;

	float exp = 1.0f;
	bool no_bvh = false;

	uint32_t film_width = -1U; //override film width (if not -1U)
	uint32_t film_height = -1U; //override film height (if not -1U)
	uint32_t film_samples = -1U; //override film samples (if not -1U)
	uint32_t film_max_ray_depth = -1U; //override film max ray depth (if not -1U)
	std::string film_sample_pattern = ""; //override film sample pattern (if not "")

	std::string write_file = ""; //write file (useful for conversions)


	CLI::App args{"Scotty3D - Student Version"};

	auto tests_option = args.add_option("--run-tests", tests_prefix, "Run all tests starting with prefix", true);
	tests_option->expected(0, 1);

	args.add_option("-s,--scene", set.scene_file, "Scene file to load");
	args.add_option("--write", write_file, "Re-save file and exit");
	args.add_flag("--trace", pathtrace, "Path trace scene without opening the GUI");
	args.add_flag("--rasterize", rasterize, "Rasterize scene without opening the GUI");
	args.add_option("-c,--camera", camera_name, "Camera instance to render (if headless)");
	args.add_option("-o,--output", output_file, "Image file to write (if headless) [for animation, can also be a directory]");
	args.add_flag("--animate", animate, "Output animation frames [min_frame,max_frame] (if headless)");
	args.add_option("--min-frame", min_frame, "First animation frame");
	args.add_option("--max-frame", max_frame, "Last animation frame (-1 is last keyframe)");
	args.add_flag("--no_bvh", no_bvh, "Don't use BVH (if headless)");
	args.add_option("--exposure", exp, "Output exposure (if headless)");
	args.add_option("--seed", RNG::fixed_seed, "Use fixed seed for RNG when rendering; (0 disables).");
	args.add_option("--film-width",          film_width, "Override camera film width (pixels)");
	args.add_option("--film-height",         film_height, "Override camera film height (pixels)");
	args.add_option("--film-samples",        film_samples, "Override film samples-per-pixel (for pathtracer)");
	args.add_option("--film-max-ray-depth",  film_max_ray_depth, "Override film max ray depth (for pathtracer)");
	args.add_option("--film-sample-pattern", film_sample_pattern, "Override film sample pattern (for rasterizer)");
	args.add_option("--force-dpi", Platform::force_dpi, "Force DPI to a given number (will scale UI).");

	CLI11_PARSE(args, argc, argv);

	// if tests are to be run, run them and return error if (some) tests fail:
	if (tests_option->count() > 0) {
		if (Test::run_tests(tests_prefix)) {
			return 0;
		} else {
			return 1;
		}
	}

	if (animate && !(pathtrace || rasterize)) {
		warn("ERROR: must specify --trace or --rasterize when doing --animate.");
		return 1;
	}

	if ((min_frame != 0 || max_frame != -1) && !animate) {
		warn("ERROR: --min-frame and --max-frame should only be used with --animate");
		return 1;
	}


	//if headless render requested, do that and return:
	if (pathtrace || rasterize || write_file != "") {
		if (set.scene_file == "") {
			warn("ERROR: must specify a scene file via --scene when doing --trace or --rasterize or --write.");
			return 1;
		}
		Scene scene;
		Animator animator;

		//load scene:
		try {
			load(set.scene_file, &scene, &animator);
		} catch (std::exception const &e) {
			warn("ERROR: Failed to load scene '%s': %s", set.scene_file.c_str(), e.what());
			return 1;
		}

		if (write_file != "") {
			info("Writing to '%s'...", write_file.c_str());
			try {
				save(write_file, scene, animator);
			} catch (std::exception const &e) {
				warn("ERROR: Failed to write scene '%s': %s", write_file.c_str(), e.what());
				return 1;
			}
			return 0;
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

		if (RNG::fixed_seed == 0) {
			RNG::fixed_seed = (std::random_device())();
		}

		//----------------------------
		//animation setup

		if (animate) {
			if (max_frame < 0) {
				max_frame = int32_t(std::ceil(animator.max_key()));
				info("Set max_frame from max_key to %d", max_frame);
			}
			if (min_frame > max_frame) {
				warn("Frame range [%d,%d] is empty!", min_frame, max_frame);
				return 1;
			}
			info("Animating frame range [%d,%d]", min_frame, max_frame);

			if (min_frame > 0) {
				info("Simulating [0,%d) to get to start frame...", min_frame);
				for (int32_t frame = 0; frame < min_frame; ++frame) {
					Scene::StepOpts opts;
					opts.reset = (frame == 0);
					opts.use_bvh = !no_bvh;
					opts.thread_pool = nullptr; //TODO
					scene.step(animator, float(frame), float(frame + 1), 1.0f / animator.frame_rate, opts);
				}
			} else {
				//just move to first frame + reset simulations:
				Scene::StepOpts opts;
				opts.reset = true;
				opts.simulate = false;
				opts.animate = false;
				scene.step(animator, 0.0f, 0.0f, 0.0f, opts);
			}
		} else {
			//render scene without driving with animation at all!
			min_frame = max_frame = 0;
		}

		//----------------------------
		//rendering loop

		info("Render settings:");
		info("\twidth: %d", camera->film.width);
		info("\theight: %d", camera->film.height);
		info("\texposure: %f", exp);
		info("\tseed: 0x%X", RNG::fixed_seed);
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
		for (int32_t frame = min_frame; frame <= max_frame; ++frame) {
			//do the render:
			info(" frame %d", frame);

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
				if (percent < 10.0f) std::cout << " ";
				std::cout << std::setprecision(2) << std::fixed;
				std::cout << percent << "%    \r";
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

				std::filesystem::path filename(output_file);

				if (animate) {
					std::stringstream str;
					str << std::setfill('0') << std::setw(4) << frame;

					std::error_code ec;
					if (std::filesystem::is_directory(filename, ec) ) {
						//numbered files within the directory:
						filename = filename / (str.str() + ".png");
					} else {
						//number goes after the stem:
						std::filesystem::path ext = filename.extension();
						filename.replace_extension("");
						filename += str.str();
						filename += ext;
					}
				}

				uint32_t data_w, data_h;
				std::vector<uint8_t> data;
				display_hdr.tonemap_to(data, exp);
				data_w = display_hdr.w;
				data_h = display_hdr.h;

				stbi_flip_vertically_on_write(true);
				if (!stbi_write_png(filename.generic_string().c_str(), data_w, data_h, 4, data.data(), data_w * 4)) {
					warn("ERROR: Failed to write output to '%s'", filename.generic_string().c_str());
					return 1;
				}
				std::cout << "Wrote result to '" << filename.generic_string() << "'." << std::endl;
			}

			//advance (if animating):
			if (animate && frame != max_frame) {
				info("Advancing %d -> %d", frame, frame + 1);
				Scene::StepOpts opts;
				opts.use_bvh = !no_bvh;
				opts.thread_pool = nullptr; //TODO
				scene.step(animator, float(frame), float(frame + 1), 1.0f / animator.frame_rate, opts);
			}

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
