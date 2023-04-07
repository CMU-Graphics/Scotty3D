#include "test.h"
#include "scene/material.h"
#include "scene/texture.h"
#include "util/rand.h"

const Spectrum tm = Spectrum{0.25f, 0.5f, 0.75f};

Test test_a3_task5_bsdf_refract_simple("a3.task5.bsdf.refract.simple", []() {

	auto tm_t = std::make_shared<Texture>(Texture{Textures::Constant{tm}});
	auto bsdf = Materials::Refract{tm_t, 1.0f};

	Vec3 out = Vec3{0.455779f, 0.870971f, -0.183507f};
	Vec3 in = Vec3{-0.455779f, -0.870971f, 0.183507f};
	RNG rng(462);

	Materials::Scatter s = bsdf.scatter(rng, out, {});

	if (Test::differs(s.direction, in)) {
		throw Test::error("Scattered Vec3{" + std::to_string(out.x) + ", " + std::to_string(out.y) + ", " + std::to_string(out.z) + "} incorrectly!\n" + 
						   "Expected Vec3{" + std::to_string(in.x) + ", " + std::to_string(in.y) + ", " + std::to_string(in.z) + "} but got "
						   + "Vec3{" + std::to_string(s.direction.x) + ", " + std::to_string(s.direction.y) + ", " + std::to_string(s.direction.z) + "} instead");
	}
	if (Test::differs(s.attenuation, 1.0f * tm)) {
		throw Test::error("Attenuation is incorrect!");
	}
});
