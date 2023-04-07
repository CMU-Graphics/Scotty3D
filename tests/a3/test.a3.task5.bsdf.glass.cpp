#include "test.h"
#include "scene/material.h"
#include "scene/texture.h"
#include "util/rand.h"

const Spectrum tm = Spectrum{0.25f, 0.5f, 0.75f};
const Spectrum rfl = Spectrum{0.9f, 0.6f, 0.3f};

Test test_a3_task5_bsdf_glass_simple("a3.task5.bsdf.glass.simple", []() {

	auto tm_t = std::make_shared<Texture>(Texture{Textures::Constant{tm}});
	auto rfl_t = std::make_shared<Texture>(Texture{Textures::Constant{rfl}});
	auto bsdf = Materials::Glass{tm_t, rfl_t, 1.0f};

	Vec3 out = Vec3{0.455779f, 0.870971f, -0.183507f};
	Vec3 in = Vec3{-0.455779f, -0.870971f, 0.183507f};

	RNG rng(462);

	Materials::Scatter s = bsdf.scatter(rng, out, {});
	if (!Test::differs(s.direction, in)) {
		if (Test::differs(s.attenuation, 1.0f * tm)) {
			throw Test::error("Attenuation is incorrect!");
		}
		return true;
	} else if (!Test::differs(s.direction, Vec3{-1.0f, 0.0f, 0.0f})) {
		if (Test::differs(s.attenuation, rfl)) {
			throw Test::error("Attenuation is incorrect!");
		}
		return false;
	}

	throw Test::error("Scattered Vec3{" + std::to_string(out.x) + ", " + std::to_string(out.y) + ", " + std::to_string(out.z) + "} incorrectly!\n" + 
						"Expected Vec3{" + std::to_string(in.x) + ", " + std::to_string(in.y) + ", " + std::to_string(in.z) + "} but got "
						+ "Vec3{" + std::to_string(s.direction.x) + ", " + std::to_string(s.direction.y) + ", " + std::to_string(s.direction.z) + "} instead");
});
