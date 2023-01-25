
#pragma once

#include <memory>
#include <functional>
#include <variant>

#include "../lib/mathlib.h"
#include "../pathtracer/samplers.h"

#include "texture.h"

namespace Environment_Lights {

//
// Environment Lights in Scotty3D provide distant lights from a range of directions.
//
// Their interface provides:
//  - sample(rng): sample a direction light might come from
//  - pdf(dir): report probability density for a given sample direction
//  - evaluate(dir): report amount of light coming from that direction
//

class Hemisphere {
public:
	Vec3 sample(RNG &rng) const;
	Spectrum evaluate(Vec3 dir) const;
	float pdf(Vec3 dir) const;

	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	Samplers::Hemisphere::Uniform sampler;

	float intensity = 1.0f;
	std::weak_ptr<Texture> radiance;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("intensity", t.intensity);
		if constexpr (I != Intent::Animate) f("radiance", t.radiance);
	}
	static inline const char *TYPE = "Hemisphere"; //used by introspect_variant<>
};

class Sphere {
public:
	Sphere() = default;
	static Sphere make_image(std::weak_ptr<Texture> image_texture);

	Vec3 sample(RNG &rng) const;
	Spectrum evaluate(Vec3 dir) const;
	float pdf(Vec3 dir) const;

	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	Samplers::Sphere::Uniform uniform;
	Samplers::Sphere::Image importance;

	float intensity = 1.0f;
	std::weak_ptr<Texture> radiance;

	//- - - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("intensity", t.intensity);
		if constexpr (I != Intent::Animate) f("radiance", t.radiance);
	}
	static inline const char *TYPE = "Sphere"; //used by introspect_variant<>
};

} // namespace Environment_Lights

class Environment_Light {
public:
	Vec3 sample(RNG &rng) const {
		return std::visit([&](auto& l) { return l.sample(rng); }, light);
	}
	Spectrum evaluate(Vec3 dir) const {
		return std::visit([&](auto& l) { return l.evaluate(dir); }, light);
	}
	float pdf(Vec3 dir) const {
		return std::visit([&](auto& l) { return l.pdf(dir); }, light);
	}

	std::weak_ptr<Texture> display() {
		return std::visit([&](auto& l) { return l.radiance; }, light);
	}

	template<typename T> bool is() const {
		return std::holds_alternative<T>(light);
	}

	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f) {
		std::visit([&](auto&& m) { m.for_each(f); }, light);
	}

	std::variant<Environment_Lights::Hemisphere, Environment_Lights::Sphere> light;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		introspect_variant< I >(std::forward< F >(f), t.light);
	}
	static inline const char *TYPE = "Environment_Light";
};

bool operator!=(const Environment_Lights::Hemisphere& a, const Environment_Lights::Hemisphere& b);
bool operator!=(const Environment_Lights::Sphere& a, const Environment_Lights::Sphere& b);
bool operator!=(const Environment_Light& a, const Environment_Light& b);
