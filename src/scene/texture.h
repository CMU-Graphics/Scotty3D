
#pragma once

#include "../lib/mathlib.h"
#include "../util/hdr_image.h"

#include <memory>
#include <variant>

class Texture;

namespace Textures {

class Image {
public:
	enum class Sampler : uint8_t {
		nearest,
		bilinear,
		trilinear,
	};
	Image() = default;
	Image(Sampler sampler_, HDR_Image const &image_);
	
	Image copy() const {
		return Image{sampler, image};
	}

	//Read value from the image.
	//  uv of [0,1]x[0,1] corresponds to the [0,w]x[0,h] of the contained image.
	//   uv outside the range is clamped to the border of the range
	//  lod is mipmap level to sample from. Ignored unless Sampler is trilinear.
	Spectrum evaluate(Vec2 uv, float lod) const;


	Sampler sampler;
	HDR_Image image;

	//updates 'levels' for current sampler and image:
	void update_mipmap();
	std::vector<HDR_Image> levels; //mipmap levels (if needed)

	GL::Tex2D to_gl() const;
};

class Constant {
public:
	Constant() = default;
	Constant(Spectrum color_) {
		float max = std::max(color_.r, std::max(color_.g, color_.b));
		if (max > 1.0f) {
			color = color_ * (1.0f / max);
			scale = max;
		} else {
			color = color_;
			scale = 1.0f;
		}
	}
	Constant(Spectrum color, float scale) : color(color), scale(scale) {
	}
	Constant copy() const {
		return Constant{color, scale};
	}

	Spectrum evaluate(Vec2 uv, float lod) const;

	Spectrum color = Spectrum(0.75f, 0.75f, 0.75f);
	float scale = 1.0f;
};

} // namespace Textures

class Texture : public std::enable_shared_from_this<Texture> {
public:
	Texture(std::variant<Textures::Image, Textures::Constant> texture)
		: texture(std::move(texture)){};
	Texture() : texture(Textures::Constant{}){};

	Texture copy() const {
		return std::visit([](auto&& t) { return Texture{t.copy()}; }, texture);
	}

	Spectrum evaluate(Vec2 uv, float lod = 0.0f) const {
		return std::visit([&](auto&& t) { return t.evaluate(uv, lod); }, texture);
	}

	template<typename T> bool is() const {
		return std::holds_alternative<T>(texture);
	}

	std::variant<Textures::Image, Textures::Constant> texture;
};

bool operator!=(const Textures::Constant& a, const Textures::Constant& b);
bool operator!=(const Textures::Image& a, const Textures::Image& b);
bool operator!=(const Texture& a, const Texture& b);
