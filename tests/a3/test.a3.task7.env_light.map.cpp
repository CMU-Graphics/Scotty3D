#include "test.h"
#include "scene/env_light.h"
#include "util/rand.h"
#include <iostream>

using Environment_Lights::Sphere;

static HDR_Image test_img() {
	return HDR_Image(12, 6,
	                 std::vector{Spectrum{0.219526f, 0.102242f, 0.291771f}, Spectrum{0.327778f, 0.124772f, 0.205079f},
	                             Spectrum{0.049707f, 0.064803f, 0.603828f}, Spectrum{0.152926f, 0.168269f, 0.584079f},
	                             Spectrum{0.208637f, 0.226966f, 0.571125f}, Spectrum{0.219526f, 0.174647f, 0.258183f},
	                             Spectrum{0.309469f, 0.230740f, 0.132868f}, Spectrum{0.024158f, 0.201556f, 0.184475f},
	                             Spectrum{0.012983f, 0.327778f, 0.056128f}, Spectrum{0.004025f, 0.054480f, 0.012286f},
	                             Spectrum{0.000000f, 0.000000f, 0.000000f}, Spectrum{0.000000f, 0.000000f, 0.000000f},
	                             Spectrum{0.327778f, 0.124772f, 0.205079f}, Spectrum{1.000000f, 0.212231f, 0.020289f},
	                             Spectrum{0.049707f, 0.064803f, 0.603828f}, Spectrum{0.208637f, 0.226966f, 0.571125f},
	                             Spectrum{0.545725f, 0.545725f, 0.545725f}, Spectrum{0.327778f, 0.238398f, 0.165132f},
	                             Spectrum{1.000000f, 0.584079f, 0.004391f}, Spectrum{0.029557f, 0.205079f, 0.262251f},
	                             Spectrum{0.015996f, 0.439657f, 0.072272f}, Spectrum{0.020289f, 0.412543f, 0.070360f},
	                             Spectrum{0.623961f, 0.009721f, 0.014444f}, Spectrum{0.266356f, 0.090842f, 0.059511f},
	                             Spectrum{0.846873f, 0.011612f, 0.017642f}, Spectrum{0.846873f, 0.011612f, 0.017642f},
	                             Spectrum{0.846873f, 0.011612f, 0.017642f}, Spectrum{0.327778f, 0.030713f, 0.171441f},
	                             Spectrum{0.049707f, 0.064803f, 0.603828f}, Spectrum{0.049707f, 0.064803f, 0.603828f},
	                             Spectrum{0.184475f, 0.304987f, 0.124772f}, Spectrum{0.262251f, 0.502887f, 0.028426f},
	                             Spectrum{0.039546f, 0.391573f, 0.149960f}, Spectrum{0.165132f, 0.258183f, 0.341915f},
	                             Spectrum{0.514918f, 0.064803f, 0.109462f}, Spectrum{0.822786f, 0.198069f, 0.132868f},
	                             Spectrum{0.665387f, 0.082283f, 0.107023f}, Spectrum{0.371238f, 0.539480f, 0.644480f},
	                             Spectrum{0.371238f, 0.539480f, 0.644480f}, Spectrum{0.533277f, 0.351533f, 0.250158f},
	                             Spectrum{0.242281f, 0.552012f, 0.194618f}, Spectrum{0.015996f, 0.439657f, 0.072272f},
	                             Spectrum{0.309469f, 0.479320f, 0.049707f}, Spectrum{1.000000f, 0.584079f, 0.004391f},
	                             Spectrum{0.246201f, 0.158961f, 0.439657f}, Spectrum{0.313989f, 0.266356f, 0.291771f},
	                             Spectrum{0.715694f, 0.226966f, 0.070360f}, Spectrum{0.450786f, 0.082283f, 0.040915f},
	                             Spectrum{0.291771f, 0.584079f, 0.327778f}, Spectrum{0.417885f, 0.428691f, 0.107023f},
	                             Spectrum{0.401978f, 0.323143f, 0.144128f}, Spectrum{0.571125f, 0.745404f, 0.381326f},
	                             Spectrum{0.863157f, 0.775822f, 0.434154f}, Spectrum{0.226966f, 0.533277f, 0.623961f},
	                             Spectrum{0.155926f, 0.300544f, 0.491021f}, Spectrum{0.246201f, 0.158961f, 0.439657f},
	                             Spectrum{0.366253f, 0.066626f, 0.371238f}, Spectrum{0.887923f, 0.219526f, 0.035601f},
	                             Spectrum{1.000000f, 0.212231f, 0.020289f}, Spectrum{0.955974f, 0.212231f, 0.025187f},
	                             Spectrum{0.386430f, 0.672443f, 0.020289f}, Spectrum{0.412543f, 0.327778f, 0.109462f},
	                             Spectrum{0.366253f, 0.066626f, 0.371238f}, Spectrum{0.412543f, 0.327778f, 0.114435f},
	                             Spectrum{0.318547f, 0.693872f, 0.822786f}, Spectrum{0.258183f, 0.366253f, 0.603828f},
	                             Spectrum{0.508881f, 0.485150f, 0.760525f}, Spectrum{0.577581f, 0.520996f, 0.799103f},
	                             Spectrum{0.623961f, 0.577581f, 0.637597f}, Spectrum{0.930111f, 0.428691f, 0.138432f},
	                             Spectrum{0.863157f, 0.313989f, 0.023153f}, Spectrum{0.854993f, 0.356400f, 0.020289f}});
}

Test test_a3_task7_env_light_map_simple("a3.task7.env_light.map.simple", []() {
	// This will test image sampling
	auto img_t = std::make_shared<Texture>(Texture{Textures::Image{Textures::Image::Sampler::bilinear, test_img()}});
	auto map = Sphere::make_image(img_t);

	RNG rng(1);

	Vec3 s = map.sample(rng);
	if (!s.valid() || s.norm() == 0.0f) {
		throw Test::error("Map produced invalid sample!");
	}
	float pdf = map.pdf(s);
	if (!std::isfinite(pdf) || pdf < 0.0f) {
		throw Test::error("Map produced sample with invalid pdf!");
	}
});

Test test_a3_task7_env_light_map_simple_evaluate("a3.task7.env_light.map.simple.evaluate", []() {
	auto img_t = std::make_shared<Texture>(Texture{Textures::Image{Textures::Image::Sampler::bilinear, test_img()}});
	auto map = Sphere::make_image(img_t);

	auto dirs = Vec3{-0.775872f, -0.614498f, 0.142882f};
	auto expected = Spectrum{0.345097f, 0.246219f, 0.232829f};
	Spectrum s = map.evaluate(dirs);
	if (Test::differs(s, expected)) {
		throw Test::error("Evaluating direction Vec3{" + std::to_string(dirs.x) + ", " +
							std::to_string(dirs.y) + ", " + std::to_string(dirs.z) + "} incorrect!");
	}
});

