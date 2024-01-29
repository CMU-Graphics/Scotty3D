
#include "pathtracer.h"
#include "../geometry/util.h"
#include "../test.h"

#include <SDL.h>
#include <thread>

namespace PT {

constexpr bool SAMPLE_AREA_LIGHTS = true;
constexpr bool RENDER_NORMALS = false;
constexpr bool LOG_CAMERA_RAYS = false;
constexpr bool LOG_AREA_LIGHT_RAYS = false;
static thread_local RNG log_rng(0x15462662); //separate RNG for logging a fraction of rays to avoid changing result when logging enabled

Spectrum Pathtracer::sample_direct_lighting_task4(RNG &rng, const Shading_Info& hit) {
	//A3T4: Pathtracer - direct light sampling (basic sampling)

	// This function computes a single-sample Monte Carlo estimate of the _direct_ lighting
	// at our ray intersection point by sampling the BSDF.

	//NOTE: this function and sample_indirect_lighting() perform very similar tasks.

    // Compute exact amount of light coming from delta lights:
	//  (these don't need to be sampled)
    Spectrum radiance = sum_delta_lights(hit);

	//TODO: ask hit.bsdf to sample an in direction that would scatter out along hit.out_dir

	//TODO: rotate that direction into world coordinates

	//TODO: construct a ray travelling in that direction
	// NOTE: because we want emitted light only, can use depth = 0 for the ray

	//TODO: trace() the ray to get the emitted light (first part of the return value)

	//TODO: weight properly depending on the probability of the sampled scattering direction and add to radiance

	return radiance;
}

Spectrum Pathtracer::sample_direct_lighting_task6(RNG &rng, const Shading_Info& hit) {
	//A3T6: Pathtracer - direct light sampling (mixture sampling)
	// TODO (PathTracer): Task 6

    // For task 6, we want to upgrade our direct light sampling procedure to also
    // sample area lights using mixture sampling.
	Spectrum radiance = sum_delta_lights(hit);

	// Example of using log_ray():
	if constexpr (LOG_AREA_LIGHT_RAYS) {
		if (log_rng.coin_flip(0.001f)) log_ray(Ray(), 100.0f);
	}

	return radiance;
}

Spectrum Pathtracer::sample_indirect_lighting(RNG &rng, const Shading_Info& hit) {
	//A3T4: path tracing - indirect lighting

	//Compute a single-sample Monte Carlo estimate of the indirect lighting contribution
	// at a given ray intersection point.

	//NOTE: this function and sample_direct_lighting_task4() perform very similar tasks.

	//TODO: ask hit.bsdf to sample an in direction that would scatter out along hit.out_dir

	//TODO: rotate that direction into world coordinates

	//TODO: construct a ray travelling in that direction
	// NOTE: be sure to reduce the ray depth! otherwise infinite recursion is possible

	//TODO: trace() the ray to get the reflected light (the second part of the return value)

	//TODO: weight properly depending on the probability of the sampled scattering direction and set radiance

	Spectrum radiance;
    return radiance;
}

std::pair<Spectrum, Spectrum> Pathtracer::trace(RNG &rng, const Ray& ray) {

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

	// TODO DEV: do we want to add ray differentials to track UV derivatives for texture sampling?
	// https://pbr-book.org/3ed-2018/Geometry_and_Transformations/Rays#RayDifferentials
	Shading_Info info = {*bsdf,         world_to_object, object_to_world, result.position, out_dir,
	                     result.normal, result.uv, ray.depth};

	Spectrum emissive = bsdf->emission(info.uv);

	//if no recursion was requested, or the material doesn't scatter light (i.e., is Materials::Emissive), don't recurse:
	if (ray.depth == 0 || bsdf->is_emissive()) return {emissive, {}};

	Spectrum direct;
	if constexpr (SAMPLE_AREA_LIGHTS) {
		direct = sample_direct_lighting_task6(rng, info);
	} else {
		direct = sample_direct_lighting_task4(rng, info);
	}

	return {emissive, direct + sample_indirect_lighting(rng, info)};
}

Pathtracer::Pathtracer() : thread_pool(std::thread::hardware_concurrency()) {
}

Pathtracer::~Pathtracer() {
	cancel();
	thread_pool.stop();
}

void Pathtracer::build_scene(Scene& scene_) {

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
			mesh_futs.emplace_back(thread_pool.enqueue([name=name,mesh=mesh,this]() {
				return std::pair{name, Tri_Mesh(Indexed_Mesh::from_halfedge_mesh( *mesh, Indexed_Mesh::SplitEdges), scene_use_bvh)};
			}));
		}

		for (const auto& [name, mesh] : scene_.skinned_meshes) {
			skinned_mesh_names[mesh] = name;
			mesh_futs.emplace_back(thread_pool.enqueue([name=name,mesh=mesh,this]() {
				return std::pair{name, Tri_Mesh(mesh->posed_mesh(), scene_use_bvh)};
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
		textures.emplace(default_texture_name, std::make_shared<Texture>(Textures::Constant{Spectrum{0.0f}, 1.0f}));

		for (const auto& [name, material] : scene_.materials) {
			material_names[material] = name;
			auto copy = std::make_shared<Material>(*material);
			copy->for_each([&](std::weak_ptr<Texture>& tex) {
				if (!tex.expired()) tex = texture_to_copy[tex.lock()];
			});
			materials.emplace(name, std::move(copy));
		}
		default_material_name = scene_.make_unique("default_material");
		materials.emplace(default_material_name, std::make_shared<Material>(Materials::Lambertian{ textures.at(default_texture_name) }));

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
							std::get<Textures::Image>(radiance->texture).image
						};
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
			Mat4 T = mesh_inst->transform.lock()->local_to_world();

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
			Mat4 T = mesh_inst->transform.lock()->local_to_world();

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
			Mat4 T = shape_inst->transform.lock()->local_to_world();

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
			//Mat4 T = part_inst->transform.lock()->local_to_world();

			auto particles = part_inst->particles.lock();
			for (const auto& p : particles->particles) {
				//NOTE: particle positions stored in world space (thus no 'T *' here):
				Mat4 pT = Mat4::translate(p.position) * Mat4::scale(Vec3{particles->radius});

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
			Mat4 T = light_inst->transform.lock()->local_to_world();

			lights.emplace_back(light.get(), T);
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

void Pathtracer::set_camera(std::shared_ptr<::Instance::Camera> camera_) {
	assert(camera_);
	assert(!camera_->camera.expired());
	assert(!camera_->transform.expired());

	camera_to_world = camera_->transform.lock()->local_to_world();
	camera = *camera_->camera.lock();
}

void Pathtracer::use_bvh(bool bvh) {
	scene_use_bvh = bvh;
}

void Pathtracer::log_ray(const Ray& ray, float t, Spectrum color) {
	std::lock_guard<std::mutex> lock(ray_log_mut);
	ray_log.push_back(Ray_Log{ray, t, color});
}

void Pathtracer::accumulate(Tile const &tile, const HDR_Image& data) {

	std::lock_guard<std::mutex> lock(accumulator_mut);

	for (uint32_t py = tile.y_begin; py < tile.y_end; ++py) {
		for (uint32_t px = tile.x_begin; px < tile.x_end; ++px) {
			uint32_t idx = py * accumulator_w + px;
			uint32_t &samples = accumulator_samples[idx];
			std::array< int64_t, 3 > &spectrum = accumulator[idx];

			//convert to 40.24 fixed point and add:
			const Spectrum& n = data.at(px, py);
			spectrum[0] += int64_t(n.r * (1ll<<24ll));
			spectrum[1] += int64_t(n.g * (1ll<<24ll));
			spectrum[2] += int64_t(n.b * (1ll<<24ll));

			//add appropriate weight:
			samples += (tile.s_end - tile.s_begin);
		}
	}
}

HDR_Image Pathtracer::accumulator_to_image() const {
	HDR_Image image(accumulator_w, accumulator_h, Spectrum(0.0f, 0.0f, 0.0f));
	for (uint32_t i = 0; i < uint32_t(accumulator.size()); ++i) {
		//(doing the conversion in double precision is probably overkill)
		if (accumulator_samples[i] > 0) {
			image.at(i) = Spectrum(
				float(accumulator[i][0] / double(1ll<<24ll) / double(accumulator_samples[i])),
				float(accumulator[i][1] / double(1ll<<24ll) / double(accumulator_samples[i])),
				float(accumulator[i][2] / double(1ll<<24ll) / double(accumulator_samples[i]))
			);
		}
	}
	return image;
}

void Pathtracer::do_trace(RNG &rng, Tile const &tile) {
	//A3T1 - Step 0: understand this function!

	HDR_Image sample(camera.film.width, camera.film.height, Spectrum(0.0f, 0.0f, 0.0f));
	for (uint32_t py = tile.y_begin; py < tile.y_end; ++py) {
		for (uint32_t px = tile.x_begin; px < tile.x_end; ++px) {
			for (uint32_t s = tile.s_begin; s < tile.s_end; ++s) {

				//generate a camera ray for this pixel:
				auto [ray, pdf] = camera.sample_ray(rng, px, py);
				ray.transform(camera_to_world);

				//if LOG_CAMERA_RAYS is set, add ray to the debug log with some small probability:
				if constexpr (LOG_CAMERA_RAYS) {
					if (log_rng.coin_flip(0.00001f)) {
						log_ray(ray, 10.0f, Spectrum{1.0f});
					}
				}

				//do path tracing:
				auto [emissive, light] = trace(rng, ray);

				Spectrum p = (emissive + light) / pdf;

				if (p.valid()) {
					sample.at(px, py) += p;
				}

				if (cancel_flag && *cancel_flag) return;
			}
		}
	}
	accumulate(tile, sample);
}

bool Pathtracer::in_progress() const {
	return traced_tiles.load() < total_tiles;
}

std::pair<float, float> Pathtracer::completion_time() const {
	return {build_timer.s(), render_timer.s()};
}

uint32_t Pathtracer::visualize_bvh(GL::Lines& lines, GL::Lines& active, uint32_t depth) {
	return scene.visualize(lines, active, depth, Mat4::I);
}

void Pathtracer::render(Scene& scene_, std::shared_ptr<::Instance::Camera> camera_,
                        std::function<void(Render_Report &&)>&& f, bool* quit,
                        bool add_samples) {
	assert(camera_);
	assert(!camera_->camera.expired());

	cancel();
	cancel_flag = quit;
	report_fn = std::move(f);

	//copy camera to local camera:
	set_camera(camera_);

	if (accumulator_w != camera_->camera.lock()->film.width || accumulator_h != camera_->camera.lock()->film.height) {
		add_samples = false;
	}

	if (!add_samples) {
		build_timer.reset();
		build_scene(scene_);
		build_timer.pause();
		accumulator_w = camera.film.width;
		accumulator_h = camera.film.height;
		std::array< int64_t, 3 > zero;
		zero.fill(0);
		accumulator.assign(accumulator_w * accumulator_h, zero);
		accumulator_samples.assign(accumulator_w * accumulator_h, 0);
		ray_log.clear();
	}
	render_timer.reset();

	//divide image into tiles for rendering:
	// (feedback will be posted back to the UI after every tile completes)
	std::vector< Tile > tiles;

	//tune these to your liking:
	// lower values == quicker feedback but also generally more overhead
	constexpr uint32_t tile_width = 100;
	constexpr uint32_t tile_height = 100;
	constexpr uint32_t tile_samples = 50;

	//get a pseudo-random stream to seed the tiles with:
	RNG seeds_rng;
	if (RNG::fixed_seed != 0) seeds_rng.seed(RNG::fixed_seed);

	for (uint32_t y_begin = 0; y_begin < camera.film.height; y_begin += tile_height) {
		uint32_t y_end = std::min(y_begin + tile_height, camera.film.height);
		for (uint32_t x_begin = 0; x_begin < camera.film.width; x_begin += tile_width) {
			uint32_t x_end = std::min(x_begin + tile_width, camera.film.width);
			for (uint32_t s_begin = 0; s_begin < camera.film.samples; s_begin += tile_samples) {
				uint32_t s_end = std::min(s_begin + tile_samples, camera.film.samples);
				uint32_t seed = seeds_rng.mt();
				tiles.emplace_back(Tile{seed, x_begin, x_end, y_begin, y_end, s_begin, s_end});
			}
		}
	}

	//a bit of flare -- do the tiles in a fancy order:
	std::stable_sort(tiles.begin(), tiles.end(), [this](Tile const &a, Tile const &b){
		//do tiles from the inside out:
		auto distance_from_center = [this](Tile const &t) {
			return Vec2(
				0.5f * (t.x_begin + t.x_end) - camera.film.width * 0.5f, 
				0.5f * (t.y_begin + t.y_end) - camera.film.height * 0.5f
			).norm();
		};
		float da = distance_from_center(a);
		float db = distance_from_center(b);
		if (da != db) return da < db;
		//make sure to do the tiles at the same location in order of s_begin:
		// (since 'accumulate' uses it for weight computation)
		return a.s_begin < b.s_begin;
	});


	//actually launch the render jobs:
	total_tiles = uint32_t(tiles.size());
	for (auto const &tile : tiles) {
		//queue up a render job per-tile:
		thread_pool.enqueue([tile, this]() {
			RNG rng(tile.seed);
			do_trace(rng, tile);

			uint32_t traced = traced_tiles.fetch_add(1) + 1;
			if (traced == total_tiles) {
				std::lock_guard<std::mutex> lock(accumulator_mut);
				render_timer.pause();
				report_fn({1.0f, accumulator_to_image()});
			} else {
				std::lock_guard<std::mutex> lock(accumulator_mut);
				report_fn({traced / float(total_tiles), accumulator_to_image()});
			}
		});
	}
}

void Pathtracer::cancel() {
	if (cancel_flag) *cancel_flag = true;
	thread_pool.clear();
	traced_tiles = 0;
	total_tiles = 0;
	if (cancel_flag) *cancel_flag = false;
	render_timer.pause();
}

const std::vector<Pathtracer::Ray_Log> Pathtracer::copy_ray_log() {
	std::lock_guard<std::mutex> lock(ray_log_mut);
	return ray_log;
}

Vec3 Pathtracer::sample_area_lights(RNG &rng, Vec3 from) {

	size_t n_emissive = emissive_objects.n_primitives();
	size_t n_env = env_lights.size();

	auto sample_env_lights = [&]() {
		int32_t n = rng.integer(0, static_cast<int32_t>(n_env));
		auto it = env_lights.begin();
		std::advance(it, n);
		return it->second->sample(rng);
	};

	if (n_emissive > 0 && n_env > 0) {
		if (rng.coin_flip(0.5f)) {
			return sample_env_lights();
		} else {
			return emissive_objects.sample(rng, from);
		}
	}
	if (n_env > 0) {
		return sample_env_lights();
	}
	return emissive_objects.sample(rng, from);
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

Spectrum Pathtracer::sum_delta_lights(const Shading_Info& hit) {

	if (hit.bsdf.is_specular()) return {};

	Spectrum radiance;
	for (auto& light : point_lights) {
		Delta_Lights::Incoming incoming = light.incoming(hit.pos);
		Vec3 in_dir = hit.world_to_object.rotate(incoming.direction);

		Spectrum attenuation = hit.bsdf.evaluate(hit.out_dir, in_dir, hit.uv);
		if (attenuation.luma() == 0.0f) continue;

		Ray shadow_ray(hit.pos, incoming.direction, Vec2{EPS_F, incoming.distance - EPS_F});

		Trace shadow = scene.hit(shadow_ray);
		if (!shadow.hit) {
			radiance += attenuation * incoming.radiance;
		}
	}

	return radiance;
}

} // namespace PT
