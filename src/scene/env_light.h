
#pragma once

#include <memory>
#include <functional>
#include <variant>

#include "../lib/mathlib.h"
#include "../pathtracer/samplers.h"

#include "texture.h"

namespace Environment_Lights {

class Hemisphere {
public:
	Vec3 sample() const;
	Spectrum evaluate(Vec3 dir) const;
	float pdf(Vec3 dir) const;

	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	Samplers::Hemisphere::Uniform sampler;

	float intensity = 1.0f;
	std::weak_ptr<Texture> radiance;
};

class Sphere {
public:
	Sphere() = default;
	static Sphere make_image(std::weak_ptr<Texture> image_texture);

	Vec3 sample() const;
	Spectrum evaluate(Vec3 dir) const;
	float pdf(Vec3 dir) const;

	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	Samplers::Sphere::Uniform uniform;
	Samplers::Sphere::Image importance;

	float intensity = 1.0f;
	std::weak_ptr<Texture> radiance;
};

} // namespace Environment_Lights

class Environment_Light {
public:
	Vec3 sample() const {
		return std::visit([&](auto& l) { return l.sample(); }, light);
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
};

bool operator!=(const Environment_Lights::Hemisphere& a, const Environment_Lights::Hemisphere& b);
bool operator!=(const Environment_Lights::Sphere& a, const Environment_Lights::Sphere& b);
bool operator!=(const Environment_Light& a, const Environment_Light& b);
