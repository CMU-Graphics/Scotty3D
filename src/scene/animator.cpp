
#include "animator.h"
#include "scene.h"

#include "../lib/mathlib.h"

template<typename F> static void channels(Camera& val, F&& f) {
	f("vertical_fov", val.vertical_fov);
	f("aspect_ratio", val.aspect_ratio);
	f("near_plane", val.near_plane);
}

template<typename F> static void channels(Instance::Geometry_Settings& val, F&& f) {
	f("visible", val.visible);
	//f("wireframe", val.draw_style); //<-- for now, let's not deal with this
}

template<typename F> static void channels(Instance::Simulate_Settings& val, F&& f) {
	f("visible", val.visible);
	f("wireframe", val.wireframe);
	f("simulate_here", val.simulate_here);
}

template<typename F> static void channels(Instance::Light_Settings& val, F&& f) {
	f("visible", val.visible);
}

template<typename F> static void channels(Instance::Mesh& val, F&& f) {
	channels(val.settings, std::forward<F&&>(f));
}

template<typename F> static void channels(Instance::Skinned_Mesh& val, F&& f) {
	channels(val.settings, std::forward<F&&>(f));
}

template<typename F> static void channels(Instance::Shape& val, F&& f) {
	channels(val.settings, std::forward<F&&>(f));
}

template<typename F> static void channels(Instance::Delta_Light& val, F&& f) {
	channels(val.settings, std::forward<F&&>(f));
}

template<typename F> static void channels(Instance::Environment_Light& val, F&& f) {
	channels(val.settings, std::forward<F&&>(f));
}

template<typename F> static void channels(Instance::Particles& val, F&& f) {
	channels(val.settings, std::forward<F&&>(f));
}

template<typename F> static void channels(Instance::Camera& val, F&& f) {
	// Camera instances currently have no channels.
}

template<typename F> static void channels(Environment_Light& val, F&& f) {
	// Env lights currently have no channels.
}

template<typename F> static void channels(Delta_Light& val, F&& f) {
	std::visit(overloaded{
				   [&](Delta_Lights::Point& val) {
					   f("color", val.color);
					   f("intensity", val.intensity);
				   },
				   [&](Delta_Lights::Directional& val) {
					   f("color", val.color);
					   f("intensity", val.intensity);
				   },
				   [&](Delta_Lights::Spot& val) {
					   f("color", val.color);
					   f("intensity", val.intensity);
					   f("inner_angle", val.inner_angle);
					   f("outer_angle", val.outer_angle);
				   },
			   },
	           val.light);
}

template<typename F> static void channels(Material& val, F&& f) {
	std::visit(overloaded{
				   [&](Materials::Lambertian& val) {},
				   [&](Materials::Mirror& val) {},
				   [&](Materials::Refract& val) { f("ior", val.ior); },
				   [&](Materials::Glass& val) { f("ior", val.ior); },
				   [&](Materials::Emissive& val) {},
			   },
	           val.material);
}

template<typename F> static void channels(Shape& val, F&& f) {
	std::visit(
		overloaded{
			[&](Shapes::Sphere& val) { f("radius", val.radius); },
		},
		val.shape);
}

template<typename F> static void channels(Particles& val, F&& f) {
	f("gravity", val.gravity);
	f("radius", val.radius);
	f("initial_velocity", val.initial_velocity);
	f("spread_angle", val.spread_angle);
	f("lifetime", val.lifetime);
	f("rate", val.rate);
}

template<typename F>
static void channels_prefix(const std::string& prefix, Skeleton::Bone& val, F&& f) {
	//these influence bind pose and shouldn't be driven during animation:
	//f(prefix + ".extent", val.extent);
	//f(prefix + ".roll", val.extent);
	//f(prefix + ".radius", val.radius);

	f(prefix + ".pose", val.pose);
}

template<typename F>
static void channels_prefix(const std::string& prefix, Skeleton::Handle& val, F&& f) {
	//hmm, not sure if these should be animatable:
	f(prefix + ".target", val.target);
	f(prefix + ".enabled", val.enabled);
}

template<typename F> static void channels(Skeleton& val, F&& f) {
	//base is part of bind pose and shouldn't be driven during animation:
	//f("base", val.base);

	f("base_offset", val.base_offset);
	for (auto &bone : val.bones) {
		channels_prefix("bone." + std::to_string(bone.channel_id), bone, std::forward<F&&>(f));
	}
	for (auto &handle : val.handles) {
		channels_prefix("handle." + std::to_string(handle.channel_id), handle, std::forward<F&&>(f));
	}
}

template<typename F> static void channels(Skinned_Mesh& val, F&& f) {
	channels(val.mesh, std::forward<F&&>(f));
	channels(val.skeleton, std::forward<F&&>(f));
}

template<typename F> static void channels(Texture& val, F&& f) {
	std::visit(overloaded{
				   [&](Textures::Image& val) {},
				   [&](Textures::Constant& val) {
					   f("color", val.color);
					   f("scale", val.scale);
				   },
			   },
	           val.texture);
}

template<typename F> static void channels(Transform& val, F&& f) {
	f("translation", val.translation);
	f("rotation", val.rotation);
	f("scale", val.scale);
}

template<typename F> static void channels(Halfedge_Mesh& val, F&& f) {
	// Halfedge meshes currently have no animation channels.
}

void Animator::merge(Animator&& other) {
	for (auto& [key, val] : other.splines) {
		splines[key] = std::move(val);
	}
	other.splines.clear();
}

template<typename T> std::optional<T> Animator::get(const Animator::Path& path, float time) const {
	auto it = splines.find(path);
	if (it == splines.end()) {
		return std::nullopt;
	}

	assert(std::holds_alternative<Spline<T>>(it->second));

	auto& spline = std::get<Spline<T>>(it->second);

	if (spline.any()) return spline.at(time);

	return std::nullopt;
}

template<typename T> void Animator::set(const Path& path, float time, T value) {
	auto it = splines.find(path);
	if (it == splines.end()) {
		Spline<T> spline;
		spline.set(time, value);
		splines.emplace(path, spline);
		return;
	}

	assert(std::holds_alternative<Spline<T>>(it->second));

	auto& spline = std::get<Spline<T>>(it->second);
	spline.set(time, value);
}

void Animator::erase(const Path& path, float time) {
	auto it = splines.find(path);
	if (it == splines.end()) return;

	bool any = true;
	std::visit(
		[&](auto& spline) {
			spline.erase(time);
			any = spline.any();
		},
		it->second);

	if (!any) {
		splines.erase(path);
	}
}

void Animator::drive(Scene& scene, float time) const {
	scene.for_each([&](const std::string& name, auto& resource) {
		channels(*resource, [&](const std::string& path, auto& value) {
			if (auto v = get<std::decay_t<decltype(value)>>(Path{name, path}, time)) {
				value = v.value();
			}
		});
	});
}

std::vector< std::pair< Animator::Path, Animator::Channel_Spline > > Animator::remove_unused_channels(Scene& scene) {
	std::unordered_set< Path > paths;

	scene.for_each([&](const std::string& name, auto& resource) {
		channels(*resource, [&](const std::string& path, auto& value) {
			auto ret = paths.emplace(Path{name, path});
			if (!ret.second) {
				warn("Channel '%s:%s' appears more than once in scene(!)", name.c_str(), path.c_str());
			}
		});
	});

	std::vector< std::pair< Path, Channel_Spline > > unused;
	for (auto i = splines.begin(); i != splines.end(); /* later */) {
		if (!paths.count(i->first)) {
			unused.emplace_back(*i);
			auto old = i;
			++i;
			splines.erase(old);
		} else {
			++i;
		}
	}
	if (!unused.empty()) {
		info("Removed %u unused channels.", uint32_t(unused.size()));
	}
	return unused;
}

void Animator::insert_channels(std::vector< std::pair< Path, Channel_Spline > > const &channels) {
	for (auto const &c : channels) {
		splines.emplace(c);
	}
}

bool Animator::has_channels(Scene& scene, const std::string& name) const {
	bool ret = false;
	scene.find(name, [&](const std::string&, auto& resource) {
		channels(*resource, [&](const std::string& path, auto&) { ret = true; });
	});
	return ret;
}

void Animator::set_all(Scene& scene, const std::string& name, float time) {

	auto set_channels = [&](auto& resource) {
		using R = std::decay_t<decltype(*resource)>;
		auto name = scene.name<R>(resource).value();
		channels(*resource, [&](const std::string& path, auto& value) {
			set(Path{name, path}, time, value);
		});
	};
	auto material = [&](auto& m) {
		set_channels(m);
		m->for_each([&](auto& texture) {
			if (auto t = texture.lock()) set_channels(t);
		});
	};

	scene.find(name, [&](const std::string&, auto& resource) {
		using I = std::decay_t<decltype(*resource)>;

		set_channels(resource);

		if constexpr (std::is_same_v<I, Instance::Mesh>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->mesh.lock()) set_channels(r);
			if (auto r = resource->material.lock()) material(r);
		} else if constexpr (std::is_same_v<I, Instance::Skinned_Mesh>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->mesh.lock()) set_channels(r);
			if (auto r = resource->material.lock()) material(r);
		} else if constexpr (std::is_same_v<I, Instance::Shape>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->shape.lock()) set_channels(r);
			if (auto r = resource->material.lock()) material(r);
		} else if constexpr (std::is_same_v<I, Instance::Delta_Light>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->light.lock()) set_channels(r);
		} else if constexpr (std::is_same_v<I, Instance::Environment_Light>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->light.lock()) set_channels(r);
		} else if constexpr (std::is_same_v<I, Instance::Particles>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->mesh.lock()) set_channels(r);
			if (auto r = resource->material.lock()) material(r);
			if (auto r = resource->particles.lock()) set_channels(r);
		} else if constexpr (std::is_same_v<I, Instance::Camera>) {
			if (auto r = resource->transform.lock()) set_channels(r);
			if (auto r = resource->camera.lock()) set_channels(r);
		}
	});
}

void Animator::erase_all(Scene& scene, const std::string& name, float time) {

	auto erase_channels = [&](auto& resource) {
		using R = std::decay_t<decltype(*resource)>;
		auto name = scene.name<R>(resource).value();
		channels(*resource, [&](const std::string& path, auto&) { erase(Path{name, path}, time); });
	};
	auto material = [&](auto& m) {
		erase_channels(m);
		m->for_each([&](auto& texture) {
			if (auto t = texture.lock()) erase_channels(t);
		});
	};

	scene.find(name, [&](const std::string&, auto& resource) {
		using I = std::decay_t<decltype(*resource)>;

		erase_channels(resource);

		if constexpr (std::is_same_v<I, Instance::Mesh>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->mesh.lock()) erase_channels(r);
			if (auto r = resource->material.lock()) material(r);
		} else if constexpr (std::is_same_v<I, Instance::Skinned_Mesh>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->mesh.lock()) erase_channels(r);
			if (auto r = resource->material.lock()) material(r);
		} else if constexpr (std::is_same_v<I, Instance::Shape>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->shape.lock()) erase_channels(r);
			if (auto r = resource->material.lock()) material(r);
		} else if constexpr (std::is_same_v<I, Instance::Delta_Light>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->light.lock()) erase_channels(r);
		} else if constexpr (std::is_same_v<I, Instance::Environment_Light>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->light.lock()) erase_channels(r);
		} else if constexpr (std::is_same_v<I, Instance::Particles>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->mesh.lock()) erase_channels(r);
			if (auto r = resource->material.lock()) material(r);
			if (auto r = resource->particles.lock()) erase_channels(r);
		} else if constexpr (std::is_same_v<I, Instance::Camera>) {
			if (auto r = resource->transform.lock()) erase_channels(r);
			if (auto r = resource->camera.lock()) erase_channels(r);
		}
	});
}

void Animator::rename(const std::string& old_name, const std::string& new_name) {
	std::unordered_map<Path, Channel_Spline> new_splines;
	for (auto& [path, spline] : splines) {
		if (path.first == old_name) {
			Path new_path(new_name, path.second);
			new_splines[new_path] = spline;
		} else {
			new_splines[path] = spline;
		}
	}
	splines = new_splines;
}

std::set<float> Animator::keys(const std::string& name) const {
	std::set<float> keys;
	for (auto& [path, spline] : splines) {
		if (path.first == name) {
			std::visit(
				[&](auto& spline) {
					auto skeys = spline.keys();
					keys.insert(skeys.begin(), skeys.end());
				},
				spline);
		}
	}
	return keys;
}

std::unordered_map<std::string, std::set<float>> Animator::all_keys() const {
	std::unordered_map<std::string, std::set<float>> keys;
	for (auto& [path, spline] : splines) {
		std::visit(
			[&, id = path.first](auto& spline) {
				auto skeys = spline.keys();
				keys[id].insert(skeys.begin(), skeys.end());
			},
			spline);
	}
	return keys;
}

void Animator::crop(float max_key) {
	for (auto& [path, spline] : splines) {
		std::visit([&](auto& spline) { spline.crop(max_key); }, spline);
	}
}

float Animator::max_key() const {
	float max_k = 0.0f;
	auto keys = all_keys();
	if (!keys.empty()) {
		for (auto& [_, k] : keys) {
			if (k.empty()) continue;
			max_k = std::max(max_k, *std::prev(k.end()));
		}
	}
	return max_k;
}

#define SPECIALIZE(T)                                                                              \
	template void Animator::set<T>(const Path&, float, T);                                         \
	template std::optional<T> Animator::get<T>(const Path&, float) const;

SPECIALIZE(bool);
SPECIALIZE(float);
SPECIALIZE(Vec2);
SPECIALIZE(Vec3);
SPECIALIZE(Vec4);
SPECIALIZE(Mat4);
SPECIALIZE(Quat);
SPECIALIZE(Spectrum);
