
#include "scene.h"
#include "animator.h"

#include <unordered_set>

void Scene::merge(Scene&& other, Animator& animator) {
	other.for_storages([this, &animator](auto& storage) {
		for (auto& [name, resource] : storage) {
			using T = std::decay_t<decltype(*resource)>;
			auto new_name = make_unique(name);
			get_storage<T>().emplace(new_name, std::move(resource));
			animator.rename(name, new_name);
		}
		storage.clear();
	});
}

bool Scene::valid() const {
	std::unordered_set< const void * > in_storage;
	auto add = [&](auto const &storage) {
		for (auto const &[k, v] : storage) {
			in_storage.emplace(v.get());
		}
	};
	add(transforms);
	add(cameras);
	add(meshes);
	add(skinned_meshes);
	add(shapes);
	add(particles);
	add(delta_lights);
	add(env_lights);
	add(textures);
	add(materials);

	//instances don't talk about each-other so not adding those.

	bool is_valid = true;

	for (auto const &[k, v] : transforms) {
		auto p = v->parent.lock();
		if (p) { //okay to be null for transforms
			if (!in_storage.count(p.get())) {
				std::cerr << "transform " << k << "'s parent is outside scene." << std::endl;
				is_valid = false;
			}
		}
	}

	for (auto const &kv : env_lights) {
		auto const &k = kv.first; auto const &v = kv.second;
		v->for_each([&](std::weak_ptr< Texture >&wt){
			auto t = wt.lock();
			if (!t) {
				is_valid = false;
				std::cerr << "env_light " << k << "'s texture is missing." << std::endl;
			} else if (!in_storage.count(t.get())) {
				is_valid = false;
				std::cerr << "env_light " << k << "'s texture is outside scene." << std::endl;
			}
		});
	}

	for (auto const &kv : materials) {
		auto const &k = kv.first; auto const &v = kv.second;
		v->for_each([&](std::weak_ptr< Texture >&wt){
			auto t = wt.lock();
			if (!t) {
				is_valid = false;
				std::cerr << "material " << k << "'s texture is missing." << std::endl;
			} else if (!in_storage.count(t.get())) {
				is_valid = false;
				std::cerr << "material" << k << "'s texture is outside scene." << std::endl;
			}
		});
	}

	#define CHECK(name, member) \
		do { \
			auto l = v->member.lock(); \
			if (!l) { \
				is_valid = false; \
				std::cerr << name " " << k << "'s " #name " is missing." << std::endl; \
			} \
		} while (0)

	for (auto const &[k, v] : instances.cameras) {
		CHECK("camera instance", transform);
		CHECK("camera instance", camera);
	}

	for (auto const &[k, v] : instances.meshes) {
		CHECK("mesh instance", transform);
		CHECK("mesh instance", mesh);
		CHECK("mesh instance", material);
	}

	for (auto const &[k, v] : instances.skinned_meshes) {
		CHECK("skinned_mesh instance", transform);
		CHECK("skinned_mesh instance", mesh);
		CHECK("skinned_mesh instance", material);
	}

	for (auto const &[k, v] : instances.shapes) {
		CHECK("shape instance", transform);
		CHECK("shape instance", shape);
		CHECK("shape instance", material);
	}

	for (auto const &[k, v] : instances.particles) {
		CHECK("particles instance", transform);
		CHECK("particles instance", mesh);
		CHECK("particles instance", material);
		CHECK("particles instance", particles);
	}

	for (auto const &[k, v] : instances.delta_lights) {
		CHECK("delta_light instance", transform);
		CHECK("delta_light instance", light);
	}

	for (auto const &[k, v] : instances.env_lights) {
		CHECK("env_light instance", transform);
		CHECK("env_light instance", light);
	}

	#undef CHECK

	return true;
}

std::unordered_set<std::string> Scene::all_names() {
	std::unordered_set<std::string> names;
	for_storages([&](auto& storage) {
		for (auto& [name, _] : storage) {
			names.insert(name);
		}
	});
	return names;
}

std::string Scene::make_unique(const std::string& name) {

	auto names = all_names();
	if (names.find(name) == names.end()) return name;

	// This is kind of dumb and inefficient
	std::string new_name = name;
	uint32_t i = 1;
	while (names.find(new_name) != names.end()) {
		new_name = name + " " + std::to_string(i);
		i++;
	}
	return new_name;
}

std::optional<std::string> Scene::rename(const std::string& before, const std::string& after) {

	auto names = all_names();
	auto it = names.find(before);
	if (it == names.end()) {
		return std::nullopt;
	}

	auto new_name = make_unique(after);

	for_storages([&](auto& storage) {
		auto entry = storage.find(before);
		if (entry != storage.end()) {
			auto resource = std::move(entry->second);
			storage.erase(entry);
			storage.emplace(new_name, std::move(resource));
		}
	});

	return new_name;
}

template<typename T> Scene::Storage<T>& Scene::get_storage() {
	if constexpr (std::is_same_v<T, Camera>)
		return cameras;
	else if constexpr (std::is_same_v<T, Delta_Light>)
		return delta_lights;
	else if constexpr (std::is_same_v<T, Environment_Light>)
		return env_lights;
	else if constexpr (std::is_same_v<T, Material>)
		return materials;
	else if constexpr (std::is_same_v<T, Shape>)
		return shapes;
	else if constexpr (std::is_same_v<T, Particles>)
		return particles;
	else if constexpr (std::is_same_v<T, Skinned_Mesh>)
		return skinned_meshes;
	else if constexpr (std::is_same_v<T, Texture>)
		return textures;
	else if constexpr (std::is_same_v<T, Transform>)
		return transforms;
	else if constexpr (std::is_same_v<T, Halfedge_Mesh>)
		return meshes;
	else if constexpr (std::is_same_v<T, Instance::Mesh>)
		return instances.meshes;
	else if constexpr (std::is_same_v<T, Instance::Skinned_Mesh>)
		return instances.skinned_meshes;
	else if constexpr (std::is_same_v<T, Instance::Shape>)
		return instances.shapes;
	else if constexpr (std::is_same_v<T, Instance::Delta_Light>)
		return instances.delta_lights;
	else if constexpr (std::is_same_v<T, Instance::Environment_Light>)
		return instances.env_lights;
	else if constexpr (std::is_same_v<T, Instance::Particles>)
		return instances.particles;
	else if constexpr (std::is_same_v<T, Instance::Camera>)
		return instances.cameras;
}

template<typename T> std::string Scene::get_type_name() {
	if constexpr (std::is_same_v<T, Camera>)
		return "Camera";
	else if constexpr (std::is_same_v<T, Delta_Light>)
		return "Delta Light";
	else if constexpr (std::is_same_v<T, Environment_Light>)
		return "Environment Light";
	else if constexpr (std::is_same_v<T, Material>)
		return "Material";
	else if constexpr (std::is_same_v<T, Shape>)
		return "Shape";
	else if constexpr (std::is_same_v<T, Particles>)
		return "Particle";
	else if constexpr (std::is_same_v<T, Skinned_Mesh>)
		return "Skinned Mesh";
	else if constexpr (std::is_same_v<T, Texture>)
		return "Texture";
	else if constexpr (std::is_same_v<T, Transform>)
		return "Transform";
	else if constexpr (std::is_same_v<T, Halfedge_Mesh>)
		return "Mesh";
	else if constexpr (std::is_same_v<T, Instance::Mesh>)
		return "Mesh Instance";
	else if constexpr (std::is_same_v<T, Instance::Skinned_Mesh>)
		return "Skinned Mesh Instance";
	else if constexpr (std::is_same_v<T, Instance::Shape>)
		return "Shape Instance";
	else if constexpr (std::is_same_v<T, Instance::Delta_Light>)
		return "Delta Light Instance";
	else if constexpr (std::is_same_v<T, Instance::Environment_Light>)
		return "Environment Light Instance";
	else if constexpr (std::is_same_v<T, Instance::Particles>)
		return "Particle Instance";
	else if constexpr (std::is_same_v<T, Instance::Camera>)
		return "Camera Instance";
}

template<typename T>
std::string Scene::insert(const std::string& name, std::shared_ptr<T>&& resource) {
	auto& storage = get_storage<T>();
	auto unique_name = make_unique(name);
	storage.emplace(unique_name, std::move(resource));
	return unique_name;
}

template<typename T> std::string Scene::create(const std::string& name, T&& resource) {
	auto& storage = get_storage<T>();
	auto unique_name = make_unique(name);
	storage.emplace(unique_name, std::make_shared<T>(std::forward<T&&>(resource)));
	return unique_name;
}

template<typename T> std::weak_ptr<T> Scene::get(const std::string& name) {
	auto& storage = get_storage<T>();
	auto it = storage.find(name);
	if (it == storage.end()) return std::weak_ptr<T>{};
	return it->second;
}

template<typename T> std::shared_ptr<T> Scene::remove(const std::string& name) {
	auto& storage = get_storage<T>();
	auto it = storage.find(name);
	if (it == storage.end()) return std::shared_ptr<T>{};
	auto result = std::move(it->second);
	storage.erase(it);
	return result;
}

std::optional<Scene::Any> Scene::get(const std::string& name) {
	std::optional<Scene::Any> result;
	for_storages([&](auto& storage) {
		auto it = storage.find(name);
		if (it != storage.end()) result = {it->second};
	});
	return result;
}

template<typename T> std::optional<std::string> Scene::name(std::weak_ptr<T> _resource) {
	auto resource = _resource.lock();
	auto& storage = get_storage<T>();
	for (auto& [name, val] : storage) {
		if (val == resource) return name;
	}
	return std::nullopt;
}

#define SPECIALIZE(T)                                                                              \
	template std::string Scene::create<T>(const std::string&, T&&);                                \
	template std::weak_ptr<T> Scene::get<T>(const std::string&);                                   \
	template std::optional<std::string> Scene::name<T>(std::weak_ptr<T>);                          \
	template Scene::Storage<T>& Scene::get_storage<T>();                                           \
	template std::string Scene::get_type_name<T>();                                                \
	template std::string Scene::insert<T>(const std::string&, std::shared_ptr<T>&&);               \
	template std::shared_ptr<T> Scene::remove<T>(const std::string&);

SPECIALIZE(Transform);
SPECIALIZE(Camera);
SPECIALIZE(Delta_Light);
SPECIALIZE(Environment_Light);
SPECIALIZE(Material);
SPECIALIZE(Shape);
SPECIALIZE(Particles);
SPECIALIZE(Skinned_Mesh);
SPECIALIZE(Texture);
SPECIALIZE(Halfedge_Mesh);

SPECIALIZE(Instance::Mesh);
SPECIALIZE(Instance::Skinned_Mesh);
SPECIALIZE(Instance::Shape);
SPECIALIZE(Instance::Delta_Light);
SPECIALIZE(Instance::Environment_Light);
SPECIALIZE(Instance::Particles);
SPECIALIZE(Instance::Camera);
