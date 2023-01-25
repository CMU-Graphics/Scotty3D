
#include "env_light.h"
#include "shape.h"

namespace Environment_Lights {

Vec3 Hemisphere::sample(RNG &rng) const {
	return sampler.sample(rng);
}

Spectrum Hemisphere::evaluate(Vec3 dir) const {
	if (dir.y < 0.0f) return {};
	return radiance.lock()->evaluate(Shapes::Sphere::uv(dir));
}

float Hemisphere::pdf(Vec3 dir) const {
	return sampler.pdf(dir);
}

void Hemisphere::for_each(const std::function<void(std::weak_ptr<Texture>&)>& f) {
	f(radiance);
}

Vec3 Sphere::sample(RNG &rng) const {
	if (radiance.lock()->is<Textures::Constant>()) return uniform.sample(rng);
	return importance.sample(rng);
}

Spectrum Sphere::evaluate(Vec3 dir) const {
	return radiance.lock()->evaluate(Shapes::Sphere::uv(dir));
}

float Sphere::pdf(Vec3 dir) const {
	if (radiance.lock()->is<Textures::Constant>()) return uniform.pdf(dir);
	return importance.pdf(dir);
}

Sphere Sphere::make_image(std::weak_ptr<Texture> image_texture) {
	Sphere ret;
	ret.radiance = image_texture;
	ret.importance =
		Samplers::Sphere::Image{std::get<Textures::Image>(image_texture.lock()->texture).image};
	return ret;
}

void Sphere::for_each(const std::function<void(std::weak_ptr<Texture>&)>& f) {
	f(radiance);
}

} // namespace Environment_Lights

bool operator!=(const Environment_Lights::Hemisphere& a, const Environment_Lights::Hemisphere& b) {
	return a.radiance.lock() != b.radiance.lock();
}

bool operator!=(const Environment_Lights::Sphere& a, const Environment_Lights::Sphere& b) {
	return a.radiance.lock() != b.radiance.lock();
}

bool operator!=(const Environment_Light& a, const Environment_Light& b) {
	if (a.light.index() != b.light.index()) return false;
	return std::visit(
		[&](const auto& light) {
			return light != std::get<std::decay_t<decltype(light)>>(b.light);
		},
		a.light);
}
