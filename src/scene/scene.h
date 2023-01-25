
#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../geometry/halfedge.h"
#include "../lib/mathlib.h"

#include "camera.h"
#include "delta_light.h"
#include "env_light.h"
#include "instance.h"
#include "material.h"
#include "particles.h"
#include "shape.h"
#include "skeleton.h"
#include "texture.h"
#include "transform.h"
#include "introspect.h"

class Animator;
class Thread_Pool;
namespace PT { class Aggregate; class Tri_Mesh; }
namespace sejp { struct value; }

/*
 * Scene represents a 3D scene with a scene graph.
 *
 * Resources in the scene are held with std::shared_ptr<>'s.
 * Resources refer to other resources via std::weak_ptr<>'s.
 *
 * NOTE: except for parent pointers in Transforms,
 *  which can be nullptr, any weak_ptr<> held by a resource
 *  must point to something else held via shared_ptr<> in the
 *  same scene, such that weak_ptr< >::lock() will always
 *  succeed.
 *
 *  So you generally don't need to write code like:
 *   if (auto val = ptr->lock()) { val->do_thing(); }
 *  And can instead just write:
 *   ptr->lock()->do_thing();
 *
 */

class Scene {
public:
	Scene() = default;
	~Scene() = default;

	Scene(const Scene&) = delete;
	Scene(Scene&&) = default;

	Scene& operator=(const Scene&) = delete;
	Scene& operator=(Scene&&) = default;

	//binary (s3ds) format:
	// Load from stream; expects stream to start with s3ds data; throws on error:
	static Scene load(std::istream& from);
	// Save to stream in s3ds format:
	void save(std::ostream& to) const;

	// Load from json (js3d) format:
	static Scene load_json(sejp::value const &from, std::string const &from_path);
	// Save as json (js3d) value:
	void save_json(std::ostream& to, std::string const &to_path) const;

	// Move all data into this scene, doing renaming where necessary.
	// Renames animation channels in rename so it can also be merged.
	void merge(Scene&& other, Animator& rename);

	// Advances scene through time:
	//  (1) if opts.reset, reset simulations and drive animations to start time (ignores animate/simulate flags)
	//  (2) build collision, step simulations
	//  (3) drive animations to end time.
	// opts may slightly modify these steps:
	struct StepOpts {
		bool reset = false; //reset simulations, drive animations to animation_from before advancing
		bool use_bvh = true; //use bvh when building aggregate for simulations
		bool animate = true; //actually advance animations?
		bool simulate = true; //actually run simulations?
		Thread_Pool *thread_pool = nullptr; //use thread pool to build bvh, if supplied
	};
	void step(Animator const &animator,
		float animate_from, float animate_to, //where to drive animation at start / end of step
		float simulate_for, //time advance for simulations during step
		StepOpts const &opts);

	//helper used by step() [and by GUI, which is why it's here]:
	//NOTE: returned collision.world [may] reference collision.meshes and this->shapes.
	struct Collision {
		PT::Aggregate world;
		std::unordered_map< Halfedge_Mesh const *, PT::Tri_Mesh > meshes;
		//NOTE: if there is a use case for Collision outliving a scene, probably should also have a copy of Shapes here
	};
	Collision build_collision(bool use_bvh, Thread_Pool *use_thread_pool = nullptr) const;

	template<typename T> std::string create(const std::string& name, T&& resource);
	template<typename T> std::weak_ptr<T> get(const std::string& name);

	template<typename T> std::string insert(const std::string& name, std::shared_ptr<T>&& resource);
	template<typename T> std::shared_ptr<T> remove(const std::string& name);

	template<typename T> std::optional<std::string> name(std::weak_ptr<T> resource);
	std::optional<std::string> rename(const std::string& before, const std::string& after);

	template<typename T> using Storage = std::unordered_map<std::string, std::shared_ptr<T>>;

	// Transforms give a hierarchy of positions in the scene:
	Storage<Transform> transforms;

	// Resources give parameters of things that can be instanced in the scene:
	Storage<Camera> cameras;
	Storage<Halfedge_Mesh> meshes;
	Storage<Skinned_Mesh> skinned_meshes;
	Storage<Shape> shapes;
	Storage<Particles> particles;
	Storage<Delta_Light> delta_lights;
	Storage<Environment_Light> env_lights;
	Storage<Texture> textures;
	Storage<Material> materials;

	// Instances include resources into the scene by associating them with transforms:
	struct Instances {
		Storage<Instance::Camera> cameras;
		Storage<Instance::Mesh> meshes;
		Storage<Instance::Skinned_Mesh> skinned_meshes;
		Storage<Instance::Shape> shapes;
		Storage<Instance::Particles> particles;
		Storage<Instance::Delta_Light> delta_lights;
		Storage<Instance::Environment_Light> env_lights;
		
		template< Intent I, typename F, typename S >
		static void introspect(F&& f, S&& s) {
			f("cameras", s.cameras);
			f("meshes", s.meshes);
			f("skinned_meshes", s.skinned_meshes);
			f("shapes", s.shapes);
			f("particles", s.particles);
			f("delta_lights", s.delta_lights);
			f("env_lights", s.env_lights);
		}
		static inline const char *TYPE = "instances";
	} instances;

	bool valid() const; //check that scene is "valid", as per block comment above (self-contained and no null references except transform.parent)

	template< Intent I, typename F, typename S >
	static void introspect(F&& f, S&& s) {
		//doing things in this order so that all references are to earlier-visited objects:
		f("transforms", s.transforms);
		f("cameras", s.cameras);
		f("meshes", s.meshes);
		f("skinned_meshes", s.skinned_meshes);
		f("shapes", s.shapes);
		f("particles", s.particles);
		f("textures", s.textures);
		f("materials", s.materials);
		f("delta_lights", s.delta_lights);
		f("env_lights", s.env_lights);

		f("instances", s.instances);

		if constexpr (I == Intent::Write) {
			if (!s.valid()) throw std::runtime_error("Scene invalidated by writing.");
		}
	}
	static inline const char *TYPE = "Scene";

	std::unordered_set<std::string> all_names();

	using Any =
		std::variant<std::weak_ptr<Transform>, std::weak_ptr<Camera>, std::weak_ptr<Delta_Light>,
	                 std::weak_ptr<Environment_Light>, std::weak_ptr<Material>,
	                 std::weak_ptr<Shape>, std::weak_ptr<Particles>, std::weak_ptr<Skinned_Mesh>,
	                 std::weak_ptr<Texture>, std::weak_ptr<Halfedge_Mesh>,
	                 std::weak_ptr<Instance::Mesh>, std::weak_ptr<Instance::Skinned_Mesh>,
	                 std::weak_ptr<Instance::Shape>, std::weak_ptr<Instance::Delta_Light>,
	                 std::weak_ptr<Instance::Environment_Light>, std::weak_ptr<Instance::Particles>,
	                 std::weak_ptr<Instance::Camera>>;

	std::optional<Any> get(const std::string& name);

	template<typename T> Storage<T>& get_storage();
	template<typename T> std::string get_type_name();

	template<typename F> void for_each(F&& f) {
		for_storages([&](auto& storage) {
			for (auto& [name, resource] : storage) {
				f(name, resource);
			}
		});
	}
	template<typename F> void find(const std::string& name, F&& f) {
		for_storages([&](auto& storage) {
			auto it = storage.find(name);
			if (it != storage.end()) {
				f(name, it->second);
			}
		});
	}

	template<typename F> void for_each_instance(F&& f) {
		for_instance_storages([&](auto& storage) {
			for (auto& [name, resource] : storage) {
				f(name, resource);
			}
		});
	}
	template<typename F> void find_instance(const std::string& name, F&& f) {
		for_instance_storages([&](auto& storage) {
			auto it = storage.find(name);
			if (it != storage.end()) {
				f(name, it->second);
			}
		});
	}

	std::string make_unique(const std::string& name);
private:

	template<typename F> void for_storages(F&& f) {
		f(transforms);
		f(cameras);
		f(meshes);
		f(skinned_meshes);
		f(shapes);
		f(particles);
		f(delta_lights);
		f(env_lights);
		f(textures);
		f(materials);
		for_instance_storages(std::forward<F>(f));
	}
	template<typename F> void for_instance_storages(F&& f) {
		f(instances.cameras);
		f(instances.meshes);
		f(instances.skinned_meshes);
		f(instances.shapes);
		f(instances.particles);
		f(instances.delta_lights);
		f(instances.env_lights);
	}
};
