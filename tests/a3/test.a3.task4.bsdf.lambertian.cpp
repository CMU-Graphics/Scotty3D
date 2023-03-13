#include "test.h"
#include "scene/material.h"
#include "scene/texture.h"
#include "util/rand.h"

Test test_a3_task4_bsdf_lambertian_simple("a3.task4.bsdf.lambertian.simple", []() {
	// This test just checks that the sample function produces a valid sample.

	auto alb_t = std::make_shared<Texture>(Texture{Textures::Constant{Spectrum{1.0f}}});
	auto bsdf = Materials::Lambertian{alb_t};

	Vec3 out;

	RNG rng(1);

	Materials::Scatter s = bsdf.scatter(rng, out, {});
	// Check that the direction is valid
	if (!s.direction.valid() || s.direction.norm() == 0.0f) {
		throw Test::error("BSDF produced invalid sample!");
	}
	float pdf = bsdf.pdf(out, s.direction);
	// Check that the pdf is valid
	if (!std::isfinite(pdf) || pdf < 0.0f) {
		throw Test::error("BSDF produced sample with invalid pdf!");
	}
	// Check the value against the exact attenuation for the first sample from RNG with seed 1
	if (Test::differs(s.attenuation, Spectrum{0.317861f, 0.317861f, 0.317861f})) {
		throw Test::error("BSDF sample attenuation was not equivalent to evaluate!");
	}
});
