
#include "pathtracer.h"
#include "../geometry/util.h"
#include "../test.h"

#include <SDL.h>
#include <thread>

namespace PT {

constexpr bool SAMPLE_AREA_LIGHTS = false;
constexpr bool RENDER_NORMALS = false;
constexpr bool LOG_CAMERA_RAYS = false;
constexpr bool LOG_AREA_LIGHT_RAYS = false;

Spectrum Pathtracer::sample_direct_lighting_task4(const Shading_Info& hit) {

	// This function computes a Monte Carlo estimate of the _direct_ lighting at our ray
    // intersection point by sampling both the BSDF and area lights.

    // Point lights are handled separately, as they cannot be intersected by tracing rays
    // into the scene.
    Spectrum radiance = point_lighting(hit);

    // TODO (PathTracer): Task 4

    // For task 4, this function should perform almost the same sampling procedure as
    // Pathtracer::sample_indirect_lighting(), but instead accumulates the emissive component of
    // incoming light (the first value returned by Pathtracer::trace()). Note that since we only
    // want emissive, we can trace a ray with depth = 0.

	return radiance;
}

Spectrum Pathtracer::sample_direct_lighting_task6(const Shading_Info& hit) {

	// TODO (PathTracer): Task 6

    // For task 6, we want to upgrade our direct light sampling procedure to also
    // sample area lights using mixture sampling.
	Spectrum radiance = point_lighting(hit);

	// Example of using log_ray():
	if constexpr (LOG_AREA_LIGHT_RAYS) {
		if (RNG::coin_flip(0.001f)) log_ray(Ray(), 100.0f);
	}

	return radiance;
}

Spectrum Pathtracer::sample_indirect_lighting(const Shading_Info& hit) {

	// TODO (PathTracer): Task 4

    // This function computes a single-sample Monte Carlo estimate of the _indirect_
    // lighting at our ray intersection point.
	Spectrum radiance;
    return radiance;
}

std::pair<Spectrum, Spectrum> Pathtracer::trace(const Ray& ray) {

	Trace result = scene.hit(ray);
	if (!result.hit) {
		if (env_lights.size()) {
			Spectrum radiance;
			for (const auto& light : env_lights) {
				radiance += light.second->evaluate(ray.dir);
			}
			return {radiance, {}};
		}
		return {};
	}

	const Material* bsdf = result.material;
	if (!bsdf) return {};

	if (!bsdf->is_sided() && dot(result.normal, ray.dir) > 0.0f) {
		result.normal = -result.normal;
	}

	if constexpr (RENDER_NORMALS) {
		return {Spectrum::direction(result.normal), {}};
	}

	Mat4 object_to_world = Mat4::rotate_to(result.normal);
	Mat4 world_to_object = object_to_world.T();
	Vec3 out_dir = world_to_object.rotate(ray.point - result.position).unit();

	// TODO: do we want to add ray differentials to track UV derivatives for texture sampling?
	// https://pbr-book.org/3ed-2018/Geometry_and_Transformations/Rays#RayDifferentials
	Shading_Info info = {*bsdf,         world_to_object, object_to_world, result.position, out_dir,
	                     result.normal, result.uv, ray.depth};

	Spectrum emissive = bsdf->emission(info.uv);

	if (ray.depth == 0) return {emissive, {}};

	Spectrum direct;
	if constexpr (SAMPLE_AREA_LIGHTS) {
		direct = sample_direct_lighting_task6(info);
	} else {
		direct = sample_direct_lighting_task4(info);
	}

	return {emissive, direct + sample_indirect_lighting(info)};
}

Pathtracer::Pathtracer() : thread_pool(std::thread::hardware_concurrency()) {
	accumulator_samples = 0;
	total_epochs = 0;
	completed_epochs = 0;
}

Pathtracer::~Pathtracer() {
	cancel();
	thread_pool.stop();
}

void Pathtracer::build_scene(Scene& scene_, std::shared_ptr<::Instance::Camera> camera) {

	// It would be nice to let the interface be usable here (as with
	// the path-tracing part), but this would cause too much hassle with
	// editing the scene while building BVHs from it.
	// This could be worked around by first copying all the mesh data
	// and then building the BVHs, but I don't think it's that big
	// of a deal, as BVH building should take at most a few seconds
	// even with many big meshes.

	// We could also do instancing instead of duplicating the bvh
	// for big meshes, but that's something to add in the future

	delta_lights.clear();
	env_lights.clear();
	textures.clear();
	materials.clear();
	meshes.clear();
	shapes.clear();
	cameras.clear();

	std::unordered_map<std::shared_ptr<Halfedge_Mesh>, std::string> mesh_names;
	std::unordered_map<std::shared_ptr<Skinned_Mesh>, std::string> skinned_mesh_names;
	std::unordered_map<std::shared_ptr<Shape>, std::string> shape_names;
	std::unordered_map<std::shared_ptr<Texture>, std::string> texture_names;
	std::unordered_map<std::shared_ptr<Material>, std::string> material_names;
	std::unordered_map<std::shared_ptr<Delta_Light>, std::string> delta_light_names;
	std::unordered_map<std::shared_ptr<Environment_Light>, std::string> env_light_names;
	std::string default_texture_name, default_material_name;

	{ // copy scene data into path tracing formats
		std::vector<std::future<std::pair<std::string, Tri_Mesh>>> mesh_futs;

		for (const auto& [name, mesh] : scene_.meshes) {
			mesh_names[mesh] = name;
			mesh_futs.emplace_back(thread_pool.enqueue([name = name, mesh = mesh]() {
				return std::pair{name, Tri_Mesh(Indexed_Mesh::from_halfedge_mesh(
										   *mesh, Indexed_Mesh::SplitEdges))};
			}));
		}

		for (const auto& [name, mesh] : scene_.skinned_meshes) {
			skinned_mesh_names[mesh] = name;
			mesh_futs.emplace_back(thread_pool.enqueue([name = name, mesh = mesh]() {
				return std::pair{name, Tri_Mesh(mesh->posed_mesh())};
			}));
		}

		for (const auto& [name, shape] : scene_.shapes) {
			shape_names[shape] = name;
			shapes.emplace(name, std::make_shared<Shape>(*shape));
		}

		std::unordered_map<std::shared_ptr<Texture>, std::shared_ptr<Texture>> texture_to_copy;
		for (const auto& [name, texture] : scene_.textures) {
			texture_names[texture] = name;
			auto copy = std::make_shared<Texture>(texture->copy());
			texture_to_copy[texture] = copy;
			textures.emplace(name, std::move(copy));
		}
		default_texture_name = scene_.make_unique("default_texture");
		textures.emplace(default_texture_name,
		                 std::make_shared<Texture>(Textures::Constant{Spectrum{0.0f}, 1.0f}));

		for (const auto& [name, material] : scene_.materials) {
			material_names[material] = name;
			auto copy = std::make_shared<Material>(*material);
			copy->for_each([&](std::weak_ptr<Texture>& tex) {
				if (!tex.expired()) tex = texture_to_copy[tex.lock()];
			});
			materials.emplace(name, std::move(copy));
		}
		default_material_name = scene_.make_unique("default_material");
		materials.emplace(default_material_name, std::make_shared<Material>(Materials::Lambertian{
													 textures.at(default_texture_name)}));

		for (const auto& [name, delta_light] : scene_.delta_lights) {
			delta_light_names[delta_light] = name;
			delta_lights.emplace(name, std::make_shared<Delta_Light>(*delta_light));
		}

		for (const auto& [name, env_light] : scene_.env_lights) {
			env_light_names[env_light] = name;
			auto light = std::make_shared<Environment_Light>(*env_light);
			light->for_each([&](std::weak_ptr<Texture>& tex) {
				if (!tex.expired()) tex = texture_to_copy[tex.lock()];
			});
			if (light->is<Environment_Lights::Sphere>()) {
				auto& sphere_map = std::get<Environment_Lights::Sphere>(light->light);
				if (auto radiance = sphere_map.radiance.lock()) {
					if (radiance->is<Textures::Image>()) {
						sphere_map.importance = Samplers::Sphere::Image{
							std::get<Textures::Image>(radiance->texture).image};
					}
				}
			}
			env_lights.emplace(name, std::move(light));
		}

		for (auto& f : mesh_futs) {
			auto [name, mesh] = f.get();
			meshes.emplace(name, std::make_shared<Tri_Mesh>(std::move(mesh)));
		}
	}

	{ // create scene instances
		std::vector<Instance> objects, area_lights;
		std::vector<Light_Instance> lights;

		for (const auto& [name, mesh_inst] : scene_.instances.meshes) {

			if (!mesh_inst->settings.visible) continue;
			if (mesh_inst->mesh.expired()) continue;

			auto& mesh = meshes.at(mesh_names.at(mesh_inst->mesh.lock()));
			auto& material = materials.at(mesh_inst->material.expired()
			                                  ? default_material_name
			                                  : material_names.at(mesh_inst->material.lock()));
			Mat4 T = mesh_inst->transform.expired() ? Mat4::I
			                                        : mesh_inst->transform.lock()->local_to_world();

			objects.emplace_back(mesh.get(), material.get(), T);

			if (material->is_emissive()) {
				area_lights.emplace_back(mesh.get(), material.get(), T);
			}
		}

		for (const auto& [name, mesh_inst] : scene_.instances.skinned_meshes) {

			if (!mesh_inst->settings.visible) continue;
			if (mesh_inst->mesh.expired()) continue;

			auto& mesh = meshes.at(skinned_mesh_names.at(mesh_inst->mesh.lock()));
			auto& material = materials.at(mesh_inst->material.expired()
			                                  ? default_material_name
			                                  : material_names.at(mesh_inst->material.lock()));
			Mat4 T = mesh_inst->transform.expired() ? Mat4::I
			                                        : mesh_inst->transform.lock()->local_to_world();

			objects.emplace_back(mesh.get(), material.get(), T);

			if (material->is_emissive()) {
				area_lights.emplace_back(mesh.get(), material.get(), T);
			}
		}

		for (const auto& [name, shape_inst] : scene_.instances.shapes) {

			if (!shape_inst->settings.visible) continue;
			if (shape_inst->shape.expired()) continue;

			auto& shape = shapes.at(shape_names.at(shape_inst->shape.lock()));
			auto& material = materials.at(shape_inst->material.expired()
			                                  ? default_material_name
			                                  : material_names.at(shape_inst->material.lock()));
			Mat4 T = shape_inst->transform.expired()
			             ? Mat4::I
			             : shape_inst->transform.lock()->local_to_world();

			objects.emplace_back(shape.get(), material.get(), T);

			if (material->is_emissive()) {
				area_lights.emplace_back(shape.get(), material.get(), T);
			}
		}

		for (const auto& [name, part_inst] : scene_.instances.particles) {

			if (!part_inst->settings.visible) continue;
			if (part_inst->mesh.expired()) continue;
			if (part_inst->particles.expired()) continue;

			auto& mesh = meshes.at(mesh_names.at(part_inst->mesh.lock()));
			auto& material = materials.at(part_inst->material.expired()
			                                  ? default_material_name
			                                  : material_names.at(part_inst->material.lock()));
			Mat4 T = part_inst->transform.expired() ? Mat4::I
			                                        : part_inst->transform.lock()->local_to_world();

			auto particles = part_inst->particles.lock();
			for (const auto& p : particles->particles) {
				Mat4 pT = T * Mat4::translate(p.position) * Mat4::scale(Vec3{particles->scale});

				objects.emplace_back(mesh.get(), material.get(), pT);
				if (material->is_emissive()) {
					area_lights.emplace_back(mesh.get(), material.get(), pT);
				}
			}
		}

		for (const auto& [name, light_inst] : scene_.instances.delta_lights) {

			if (!light_inst->settings.visible) continue;
			if (light_inst->light.expired()) continue;

			auto& light = delta_lights.at(delta_light_names.at(light_inst->light.lock()));
			Mat4 T = light_inst->transform.expired()
			             ? Mat4::I
			             : light_inst->transform.lock()->local_to_world();

			lights.emplace_back(light.get(), T);
		}

		{
			assert(camera);
			assert(!camera->camera.expired());

			Mat4 T =
				camera->transform.expired() ? Mat4::I : camera->transform.lock()->local_to_world();
			cam_param = Camera_Instance{*camera->camera.lock(), T};
		}

		emissive_objects = List(std::move(area_lights));
		point_lights = std::move(lights);

		if (scene_use_bvh) {
			scene = Aggregate(BVH<Instance>(std::move(objects)));
		} else {
			scene = Aggregate(List<Instance>(std::move(objects)));
		}
	}
}

void Pathtracer::use_bvh(bool bvh) {
	scene_use_bvh = bvh;
}

void Pathtracer::log_ray(const Ray& ray, float t, Spectrum color) {
	std::lock_guard<std::mutex> lock(ray_log_mut);
	ray_log.push_back(Ray_Log{ray, t, color});
}

void Pathtracer::accumulate(const HDR_Image& sample) {

	std::lock_guard<std::mutex> lock(accumulator_mut);

	accumulator_samples++;
	for (uint32_t j = 0; j < cam_param.c.film.height; j++) {
		for (uint32_t i = 0; i < cam_param.c.film.width; i++) {
			Spectrum& s = accumulator.at(i, j);
			const Spectrum& n = sample.at(i, j);
			s += (n - s) * (1.0f / accumulator_samples);
		}
	}
}

void Pathtracer::do_trace(uint32_t samples) {

	HDR_Image sample(cam_param.c.film.width, cam_param.c.film.height);
	for (uint32_t j = 0; j < cam_param.c.film.height; j++) {
		for (uint32_t i = 0; i < cam_param.c.film.width; i++) {

			uint32_t sampled = 0;
			for (uint32_t s = 0; s < samples; s++) {

				auto [ray, pdf] = cam_param.c.generate_ray<Samplers::Rect>(i, j);
				ray.transform(cam_param.iV);

				if constexpr (LOG_CAMERA_RAYS) {
					if (RNG::coin_flip(0.00001f)) {
						log_ray(ray, 10.0f, Spectrum{1.0f});
					}
				}

				auto [emissive, light] = trace(ray);

				Spectrum p = (emissive + light) / pdf;

				if (p.valid()) {
					sample.at(i, j) += p;
					sampled++;
				}

				if (cancel_flag && *cancel_flag) return;
			}

			if (sampled > 0) sample.at(i, j) *= (1.0f / sampled);
		}
	}
	accumulate(sample);
}

bool Pathtracer::in_progress() const {
	return completed_epochs.load() < total_epochs;
}

std::pair<float, float> Pathtracer::completion_time() const {
	return {build_timer.s(), render_timer.s()};
}

uint32_t Pathtracer::visualize_bvh(GL::Lines& lines, GL::Lines& active, uint32_t depth) {
	return scene.visualize(lines, active, depth, Mat4::I);
}

void Pathtracer::render(Scene& scene_, std::shared_ptr<::Instance::Camera> camera,
                        std::function<void(Pathtracer::Render_Report)>&& f, bool* quit,
                        bool add_samples) {

	assert(camera);
	assert(!camera->camera.expired());
	auto cam = camera->camera.lock();

	uint32_t n_samples = cam->film.samples;
	uint32_t n_threads = std::thread::hardware_concurrency();
	uint32_t samples_per_epoch = std::max(1u, n_samples / (n_threads * 10));

	cancel();
	cancel_flag = quit;
	report_fn = std::move(f);

	total_epochs = n_samples / samples_per_epoch + !!(n_samples % samples_per_epoch);

	auto [aw, ah] = accumulator.dimension();
	if (aw != cam->film.width || ah != cam->film.height) {
		add_samples = false;
	}

	if (!add_samples) {
		accumulator = HDR_Image(cam->film.width, cam->film.height, Spectrum(0.0f, 0.0f, 0.0f));
		accumulator_samples = 0;
		build_timer.reset();
		build_scene(scene_, camera);
		build_timer.pause();
		ray_log.clear();
	}
	render_timer.reset();

	for (uint32_t s = 0; s < n_samples; s += samples_per_epoch) {
		uint32_t samples = (s + samples_per_epoch) > n_samples ? n_samples - s : samples_per_epoch;
		thread_pool.enqueue([samples, this]() {
			do_trace(samples);
			uint32_t completed = completed_epochs++ + 1;
			if (completed == total_epochs) {
				render_timer.pause();
				report_fn({1.0f, accumulator.copy()});
			} else if (completed % std::max(total_epochs / 10, 1u) == 0) {
				std::lock_guard<std::mutex> lock(accumulator_mut);
				report_fn({static_cast<float>(completed) / static_cast<float>(total_epochs),
				           accumulator.copy()});
			}
		});
	}
}

void Pathtracer::cancel() {
	if (cancel_flag) *cancel_flag = true;
	thread_pool.clear();
	completed_epochs = 0;
	total_epochs = 0;
	if (cancel_flag) *cancel_flag = false;
	render_timer.pause();
}

const std::vector<Pathtracer::Ray_Log>& Pathtracer::get_ray_log() const {
	return ray_log;
}

Vec3 Pathtracer::sample_area_lights(Vec3 from) {

	size_t n_emissive = emissive_objects.n_primitives();
	size_t n_env = env_lights.size();

	auto sample_env_lights = [&]() {
		int32_t n = RNG::integer(0, static_cast<int32_t>(n_env));
		auto it = env_lights.begin();
		std::advance(it, n);
		return it->second->sample();
	};

	if (n_emissive > 0 && n_env > 0) {
		if (RNG::coin_flip(0.5f)) {
			return sample_env_lights();
		} else {
			return emissive_objects.sample(from);
		}
	}
	if (n_env > 0) {
		return sample_env_lights();
	}
	return emissive_objects.sample(from);
}

float Pathtracer::area_lights_pdf(Vec3 from, Vec3 dir) {

	size_t n_emissive = emissive_objects.n_primitives();
	size_t n_env = env_lights.size();

	auto env_lights_pdf = [&]() {
		float pdf = 0.0f;
		for (auto& light : env_lights) {
			pdf += light.second->pdf(dir);
		}
		return n_env ? pdf / n_env : 0.0f;
	};

	uint32_t n_strategies = (n_emissive > 0) + (n_env > 0);
	float pdf = emissive_objects.pdf(Ray(from, dir)) + env_lights_pdf();

	return n_strategies ? pdf / n_strategies : 0.0f;
}

Spectrum Pathtracer::point_lighting(const Shading_Info& hit) {

	if (hit.bsdf.is_specular()) return {};

	Spectrum radiance;
	for (auto& light : point_lights) {
		Delta_Lights::Sample sample = light.sample(hit.pos);
		Vec3 in_dir = hit.world_to_object.rotate(sample.direction);

		Spectrum attenuation = hit.bsdf.evaluate(hit.out_dir, in_dir, hit.uv);
		if (attenuation.luma() == 0.0f) continue;

		Ray shadow_ray(hit.pos, sample.direction, Vec2{EPS_F, sample.distance - EPS_F});

		Trace shadow = scene.hit(shadow_ray);
		if (!shadow.hit) {
			radiance += attenuation * sample.radiance;
		}
	}

	return radiance;
}

} // namespace PT
