
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

	struct PTInfo {
		float percent_done;
		HDR_Image current_image;
		std::vector<Ray_Log> ray_logs;
	};

	Pathtracer();
	~Pathtracer();

	void use_bvh(bool use_bvh);
	uint32_t visualize_bvh(GL::Lines& lines, GL::Lines& active, uint32_t level);
	const std::vector<Ray_Log>& get_ray_log() const;

	using Render_Report = std::pair<float, HDR_Image>;
	void render(Scene& scene, std::shared_ptr<::Instance::Camera> camera,
	            std::function<void(Render_Report)>&& f, bool* quit, bool add_samples = false);
	
	bool in_progress() const;
	std::pair<float, float> completion_time() const;

	template<typename Sampler> std::pair<Ray, float> pixel_to_ray(uint32_t x, uint32_t y);
	Spectrum sample_direct_lighting_task4(const Shading_Info& hit);
	Spectrum sample_direct_lighting_task6(const Shading_Info& hit);
	Spectrum sample_indirect_lighting(const Shading_Info& hit);

	void build_scene(Scene& scene, std::shared_ptr<::Instance::Camera> camera_name);

private:
	void cancel();
	void do_trace(uint32_t samples);
	void accumulate(const HDR_Image& sample);

	bool* cancel_flag = nullptr;
	std::function<void(Render_Report)> report_fn;

	Thread_Pool thread_pool;
	bool scene_use_bvh = true;
	Timer render_timer, build_timer;

	HDR_Image accumulator;
	std::mutex accumulator_mut;
	uint32_t total_epochs, accumulator_samples;
	std::atomic<uint32_t> completed_epochs;

	std::pair<Spectrum, Spectrum> trace(const Ray& ray);
	Spectrum point_lighting(const Shading_Info& hit);
	Vec3 sample_area_lights(Vec3 from);
	float area_lights_pdf(Vec3 from, Vec3 dir);

	std::mutex ray_log_mut;
	std::vector<Ray_Log> ray_log;
	void log_ray(const Ray& ray, float t, Spectrum color = Spectrum{1.0f});

	Aggregate scene;
	List<Instance> emissive_objects;
	std::vector<Light_Instance> point_lights;

	struct Camera_Instance {
		Camera c;
		Mat4 iV;
	};
	Camera_Instance cam_param;

	std::unordered_map<std::string, std::shared_ptr<Delta_Light>> delta_lights;
	std::unordered_map<std::string, std::shared_ptr<Environment_Light>> env_lights;
	std::unordered_map<std::string, std::shared_ptr<Material>> materials;
	std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
	std::unordered_map<std::string, std::shared_ptr<Tri_Mesh>> meshes;
	std::unordered_map<std::string, std::shared_ptr<Shape>> shapes;
	std::unordered_map<std::string, std::shared_ptr<Camera>> cameras;
};

} // namespace PT
