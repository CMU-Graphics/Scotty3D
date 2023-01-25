
#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "../lib/mathlib.h"
#include "../scene/scene.h"

#include "../util/hdr_image.h"
#include "../util/thread_pool.h"
#include "../util/timer.h"

#include "aggregate.h"

namespace PT {

class Pathtracer {
public:
	struct Shading_Info {
		const Material& bsdf;
		Mat4 world_to_object, object_to_world;
		Vec3 pos, out_dir, normal;
		Vec2 uv;
		uint32_t depth = 0;
	};
	struct Ray_Log {
		Ray ray;
		float t = 1.0f;
		Spectrum color = Spectrum{1.0f};
	};

	Pathtracer();
	~Pathtracer();

	void use_bvh(bool use_bvh);
	uint32_t visualize_bvh(GL::Lines& lines, GL::Lines& active, uint32_t level);
	const std::vector<Ray_Log> copy_ray_log(); //copy ray log (with proper locking)

	using Render_Report = std::pair<float, HDR_Image>;
	void render(Scene& scene, std::shared_ptr<::Instance::Camera> camera,
	            std::function<void(Render_Report &&)>&& f, bool* quit, bool add_samples = false);
	
	bool in_progress() const;
	std::pair<float, float> completion_time() const;

	Spectrum sample_direct_lighting_task4(RNG &rng, const Shading_Info& hit);
	Spectrum sample_direct_lighting_task6(RNG &rng, const Shading_Info& hit);
	Spectrum sample_indirect_lighting(RNG &rng, const Shading_Info& hit);

	void build_scene(Scene& scene);
	void set_camera(std::shared_ptr<::Instance::Camera> camera); //in its own function so test code can call it

private:
	void cancel();

	//a 'Tile' is a region of the image (in both pixel and sample space) to trace:
	struct Tile {
		uint32_t seed = 0; //RNG seed to use
		uint32_t x_begin = 0, x_end = 0;
		uint32_t y_begin = 0, y_end = 0;
		uint32_t s_begin = 0, s_end = 0;
	};

	//trace [x_begin,x_end)x[y_begin,y_end) region of the image, shooting rays for samples [s_begin,s_end):
	void do_trace(RNG &rng, Tile const &tile);
	//accumulate samples from do_trace into the accumulator:
	void accumulate(Tile const &tile, const HDR_Image& data);

	bool* cancel_flag = nullptr;
	std::function<void(Render_Report &&)> report_fn;

	Thread_Pool thread_pool;
	bool scene_use_bvh = true;
	Timer render_timer, build_timer;

	std::mutex accumulator_mut;
	uint32_t accumulator_w = 0, accumulator_h = 0;
	//accumulator will store spectrums as 40.24 fixed point to avoid order-of-addition nondeterminism:
	std::vector< std::array< int64_t, 3 > > accumulator;
	//accumulator will store sample counts as well:
	std::vector< uint32_t > accumulator_samples;
	//compute image (divide spectrums by sample counts):
	HDR_Image accumulator_to_image() const;

	uint32_t total_tiles = 0;
	std::atomic<uint32_t> traced_tiles = 0;

	//trace a single ray into the scene,
	//return (emitted, reflected) light incoming along ray
	std::pair<Spectrum, Spectrum> trace(RNG &rng, const Ray& ray);

	//compute the contribution of all of the delta lights in the scene:
	// NOTE: no sampling required because delta lights are in exactly one spot!
	Spectrum sum_delta_lights(const Shading_Info& hit);

	//compute a direction to one of the area lights:
	Vec3 sample_area_lights(RNG &rng, Vec3 from);
	float area_lights_pdf(Vec3 from, Vec3 dir);

	std::mutex ray_log_mut;
	std::vector<Ray_Log> ray_log;
	void log_ray(const Ray& ray, float t, Spectrum color = Spectrum{1.0f});

	Aggregate scene;
	List<Instance> emissive_objects;
	std::vector<Light_Instance> point_lights;

	Camera camera;
	Mat4 camera_to_world;

	std::unordered_map<std::string, std::shared_ptr<Delta_Light>> delta_lights;
	std::unordered_map<std::string, std::shared_ptr<Environment_Light>> env_lights;
	std::unordered_map<std::string, std::shared_ptr<Material>> materials;
	std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
	std::unordered_map<std::string, std::shared_ptr<Tri_Mesh>> meshes;
	std::unordered_map<std::string, std::shared_ptr<Shape>> shapes;
};

} // namespace PT
