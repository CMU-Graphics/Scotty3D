
#pragma once

#include <functional>
#include <memory>
#include <variant>

#include "../lib/mathlib.h"
#include "../pathtracer/samplers.h"

#include "texture.h"

namespace Materials {

//All materials in Scotty3D are represented by their BSDF (bidirectional scattering distribution function).
//
//They provide the follow interface to the BSDF:
// evaluate(out, in, uv):
//  compute attenuation when light coming from `in` direction is reflected in `out` direction at surface location `uv`.
// scatter(rng, out, uv):
//  sample direction to incoming light that might reflect in `out` direction at surface location `uv`. (also reports attenuation)
// pdf(out, in):
//  report probability density (or mass, for discrete distributions) for scattering 
// emission(uv):
//  report uniform emission from the surface at location `uv`.
//
//NOTE: that these functions always talk about directions *to* lights.
// (particularly, for incoming light, this is opposite the direction the light is traveling.)
//
//NOTE2: These functions work in surface-local coordinates, where the surface normal is (0,1,0).
//

//helpers:
Vec3 reflect(Vec3 dir);
Vec3 refract(Vec3 out_dir, float index_of_refraction, bool& was_internal);
float schlick(Vec3 in_dir, float index_of_refraction);

class Scatter {
public:
	Vec3 direction;
	Spectrum attenuation;
	bool specular = false;
};

class Lambertian {
public:
	Lambertian() = default;
	Lambertian(std::weak_ptr<Texture> albedo) : albedo(albedo) {
	}

	Spectrum evaluate(Vec3 out, Vec3 in, Vec2 uv) const;
	Scatter scatter(RNG &rng, Vec3 out, Vec2 uv) const;
	float pdf(Vec3 out, Vec3 in) const;
	Spectrum emission(Vec2 uv) const;

	constexpr bool is_emissive() const { return false; }
	constexpr bool is_specular() const { return false; }
	constexpr bool is_sided() const { return false; }

	std::weak_ptr<Texture> display() const;
	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	std::weak_ptr<Texture> albedo;
};

class Mirror {
public:
	Spectrum evaluate(Vec3 out, Vec3 in, Vec2 uv) const;
	Scatter scatter(RNG &rng, Vec3 out, Vec2 uv) const;
	float pdf(Vec3 out, Vec3 in) const;
	Spectrum emission(Vec2 uv) const;

	constexpr bool is_emissive() const { return false; }
	constexpr bool is_specular() const { return true; }
	constexpr bool is_sided() const { return false; }

	std::weak_ptr<Texture> display() const;
	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	std::weak_ptr<Texture> reflectance;
};

class Refract {
public:
	Spectrum evaluate(Vec3 out, Vec3 in, Vec2 uv) const;
	Scatter scatter(RNG &rng, Vec3 out, Vec2 uv) const;
	float pdf(Vec3 out, Vec3 in) const;
	Spectrum emission(Vec2 uv) const;

	bool is_emissive() const;
	bool is_specular() const;
	bool is_sided() const;

	std::weak_ptr<Texture> display() const;
	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	std::weak_ptr<Texture> transmittance;
	float ior = 1.5f;
};

class Glass {
public:
	Spectrum evaluate(Vec3 out, Vec3 in, Vec2 uv) const;
	Scatter scatter(RNG &rng, Vec3 out, Vec2 uv) const;
	float pdf(Vec3 out, Vec3 in) const;
	Spectrum emission(Vec2 uv) const;

	bool is_emissive() const;
	bool is_specular() const;
	bool is_sided() const;

	std::weak_ptr<Texture> display() const;
	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	std::weak_ptr<Texture> transmittance, reflectance;
	float ior = 1.5f;
};

class Emissive {
public:
	Spectrum evaluate(Vec3 out, Vec3 in, Vec2 uv) const;
	Scatter scatter(RNG &rng, Vec3 out, Vec2 uv) const;
	float pdf(Vec3 out, Vec3 in) const;
	Spectrum emission(Vec2 uv) const;

	bool is_emissive() const;
	bool is_specular() const;
	bool is_sided() const;

	std::weak_ptr<Texture> display() const;
	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f);

	std::weak_ptr<Texture> emissive;
};

} // namespace Materials

class Material {
public:
	Material(std::variant<Materials::Lambertian, Materials::Mirror, Materials::Refract,
	                      Materials::Glass, Materials::Emissive>
	             material)
		: material(std::move(material)) {
	}
	Material() : material(Materials::Lambertian{}) {
	}

	Spectrum evaluate(Vec3 out, Vec3 in, Vec2 uv) const {
		return std::visit([&](auto&& m) { return m.evaluate(out, in, uv); }, material);
	}
	Materials::Scatter scatter(RNG &rng, Vec3 out, Vec2 uv) const {
		return std::visit([&](auto&& m) { return m.scatter(rng, out, uv); }, material);
	}
	float pdf(Vec3 out, Vec3 in) const {
		return std::visit([&](auto&& m) { return m.pdf(out, in); }, material);
	}
	Spectrum emission(Vec2 uv) const {
		return std::visit([&](auto&& m) { return m.emission(uv); }, material);
	}

	bool is_emissive() const {
		return std::visit([&](auto&& m) { return m.is_emissive(); }, material);
	}
	bool is_specular() const {
		return std::visit([&](auto&& m) { return m.is_specular(); }, material);
	}
	bool is_sided() const {
		return std::visit([&](auto&& m) { return m.is_sided(); }, material);
	}

	std::weak_ptr<Texture> display() const {
		return std::visit([&](auto&& m) { return m.display(); }, material);
	}
	void for_each(const std::function<void(std::weak_ptr<Texture>&)>& f) {
		std::visit([&](auto&& m) { m.for_each(f); }, material);
	}

	template<typename T> bool is() const {
		return std::holds_alternative<T>(material);
	}

	std::variant<Materials::Lambertian, Materials::Mirror, Materials::Refract, Materials::Glass,
	             Materials::Emissive>
		material;
};

bool operator!=(const Materials::Lambertian& a, const Materials::Lambertian& b);
bool operator!=(const Materials::Mirror& a, const Materials::Mirror& b);
bool operator!=(const Materials::Refract& a, const Materials::Refract& b);
bool operator!=(const Materials::Glass& a, const Materials::Glass& b);
bool operator!=(const Materials::Emissive& a, const Materials::Emissive& b);
bool operator!=(const Material& a, const Material& b);
