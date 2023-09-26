#include "test.h"

#include <iostream>
#include "rasterizer/programs.h"

//build a testing mipmap where (r,g) have sample texcoord and (b) has level index:
[[maybe_unused]]
static Textures::Image test_mipmap_texture() {
	Textures::Image ret;
	ret.sampler = Textures::Image::Sampler::trilinear;

	auto set_colors = [](HDR_Image &image, uint32_t level) {
		for (uint32_t y = 0; y < image.h; ++y) {
			for (uint32_t x = 0; x < image.w; ++x) {
				image.at(x,y) = Spectrum((x + 0.5f) / image.w, (y + 0.5f) / image.h, float(level));
			}
		}
	};

	ret.image = HDR_Image(256, 512);
	set_colors(ret.image, 0);

	uint32_t w = ret.image.w;
	uint32_t h = ret.image.h;
	ret.levels.resize(9);
	for (uint32_t l = 0; l < ret.levels.size(); ++l) {
		assert(w > 1u || h > 1u);
		w = std::max(1u, w / 2u);
		h = std::max(1u, h / 2u);
		ret.levels[l] = HDR_Image(w,h);
		set_colors(ret.levels[l], l+1);
	}
	assert(w == 1 && h == 1);

	assert(ret.image.w > 0 && ret.image.h > 0);
	for (auto const &l : ret.levels) {
		assert(l.w > 0 && l.h > 0);
	};

	return ret;
}

//-------------------------------------------
//check image sampling functions:

//function prototypes, since these appear in texture.cpp but not texture.h:
namespace Textures {
	Spectrum sample_nearest(HDR_Image const &image, Vec2 uv);
	Spectrum sample_bilinear(HDR_Image const &image, Vec2 uv);
	Spectrum sample_trilinear(HDR_Image const &image, std::vector< HDR_Image > const &levels, Vec2 uv, float lod);
	void generate_mipmap(HDR_Image const &base, std::vector< HDR_Image > *levels_);
}

//for convenience:
Spectrum const R(1.0f, 0.0f, 0.0f);
Spectrum const G(0.0f, 1.0f, 0.0f);
Spectrum const B(0.0f, 0.0f, 1.0f);
Spectrum const W(1.0f, 1.0f, 1.0f);

//- - - - - - - - -
//nearest neighbor

Test test_a1_task6_nearest_simple("a1.task6.sample.nearest.simple", []() {
	//check that nearest does actually return nearest sample:

	HDR_Image image(3,5, std::vector< Spectrum >{
		R, R, R,
		G, G, G,
		R, B, R,
		G, W, G,
		B, B, B
	});

	auto expect_spectrum = [&](std::string const &desc, Vec2 uv, Spectrum expected){
		Spectrum got = Textures::sample_nearest(image, uv);

		if (Test::differs(got, expected)) {
			std::string params;
			params += "case '" + desc + "':\n";
			params += "        uv: " + to_string(uv) + "\n";
			params += "  expected: " + to_string(expected) + "\n";
			params += "       got: " + to_string(got);

			puts("");
			info("%s",params.c_str());

			throw Test::error("Got unexpected color in '" + desc + "' case.");
		}
	};

	Vec2 px_to_uv(1.0f / image.w, 1.0f / image.h);

	expect_spectrum("at center", px_to_uv * Vec2(1.5f, 3.5f), W);
	expect_spectrum("low corner", px_to_uv * Vec2(1.1f, 3.1f), W);
	expect_spectrum("high corner", px_to_uv * Vec2(1.9f, 3.9f), W);
});



//- - - - - - - - -
//bilinear

Test test_a1_task6_bilinear_simple("a1.task6.sample.bilinear.simple", []() {
	//check bilinear sampling is mixing the right stuff:

	HDR_Image image(3,5, std::vector< Spectrum >{
		R, R, R,
		G, G, G,
		R, B, R,
		G, W, G,
		B, B, B
	});

	auto expect_spectrum = [&](std::string const &desc, Vec2 uv, Spectrum expected){
		Spectrum got = Textures::sample_bilinear(image, uv);

		if (Test::differs(got, expected)) {
			std::string params;
			params += "case '" + desc + "':\n";
			params += "        uv: " + to_string(uv) + "\n";
			params += "  expected: " + to_string(expected) + "\n";
			params += "       got: " + to_string(got);

			puts("");
			info("%s",params.c_str());

			throw Test::error("Got unexpected color in '" + desc + "' case.");
		}
	};

	Vec2 px_to_uv(1.0f / image.w, 1.0f / image.h);

	expect_spectrum("at center", px_to_uv * Vec2(1.5f, 3.5f), W);
	expect_spectrum("linear x", px_to_uv * Vec2(1.25f, 2.5f), Spectrum(0.25f, 0.0f, 0.75f) );
	expect_spectrum("linear y", px_to_uv * Vec2(0.5f, 2.75f), Spectrum(0.75f, 0.25f, 0.0f) );
	expect_spectrum("bilinear xy", px_to_uv * Vec2(2.4f, 3.25f), Spectrum(0.3f, 0.75f, 0.1f) );
});



//- - - - - - - - -
//trilinear

Test test_a1_task6_trilinear_simple("a1.task6.sample.trilinear.simple", []() {
	//check trilinear sampling is mixing the right stuff:

	HDR_Image image(3,5, std::vector< Spectrum >{
		R, R, R,
		R, R, R,
		R, R, R,
		R, R, R,
		R, R, R
	});
	std::vector< HDR_Image > levels;
	levels.emplace_back(1,2, std::vector< Spectrum >{
		G,
		G
	});
	levels.emplace_back(1,1, std::vector< Spectrum >{
		B
	});

	auto expect_spectrum = [&](std::string const &desc, Vec2 uv, float lod, Spectrum expected){
		Spectrum got = Textures::sample_trilinear(image, levels, uv, lod);

		if (Test::differs(got, expected)) {
			std::string params;
			params += "case '" + desc + "':\n";
			params += "        uv: " + to_string(uv) + "\n";
			params += "       lod: " + std::to_string(lod) + "\n";
			params += "  expected: " + to_string(expected) + "\n";
			params += "       got: " + to_string(got);

			puts("");
			info("%s",params.c_str());

			throw Test::error("Got unexpected color in '" + desc + "' case.");
		}
	};

	expect_spectrum("base", Vec2(0.5f, 0.5f), 0.0f, R);
	expect_spectrum("levels[0]", Vec2(0.5f, 0.5f), 1.0f, G);
	expect_spectrum("levels[1]", Vec2(0.5f, 0.5f), 2.0f, B);
	expect_spectrum("base - levels[0]", Vec2(0.5f, 0.5f), 0.25f, Spectrum(0.75f, 0.25f, 0.0f));
	expect_spectrum("levels[0] - levels[1]", Vec2(0.5f, 0.5f), 1.7f, Spectrum(0.0f, 0.3f, 0.7f));
});


//-------------------------------------------
//check mipmap generation:

Test test_a1_task6_generate_mipmap("a1.task6.generate_mipmap", []() {
	HDR_Image image( 4, 6, std::vector< Spectrum >{
		R, R, B, B,
		R, R, B, B,
		B, B, R, R,
		B, B, R, R,
		R, R, B, B,
		R, R, B, B
	});

	std::vector< HDR_Image > levels;
	Textures::generate_mipmap(image, &levels);

	std::vector< std::pair< uint32_t, uint32_t > > expected{
		{2,3},
		{1,1},
	};

	//has the right number of levels:
	if (levels.size() != expected.size()) {
		throw Test::error("Image of size " + std::to_string(image.w) + "x" + std::to_string(image.h) + " should have " + std::to_string(expected.size()) + " levels, but generated " + std::to_string(levels.size()) + ".");
	}

	//has right level sizes:
	for (uint32_t l = 0; l < expected.size(); ++l) {
		if (levels[l].w != expected[l].first || levels[l].h != expected[l].second) {
			throw Test::error("Image of size " + std::to_string(image.w) + "x" + std::to_string(image.h) + " should have levels[" + std::to_string(l) + "] of size " + std::to_string(expected[l].first) + "x" + std::to_string(expected[l].second) + " but generated level of size " + std::to_string(levels[l].w) + "x" + std::to_string(levels[l].h) + ".");
		}
	}

	//is pretty close to averaging the image color in the last level:
	if (Test::differs(levels.back().at(0,0), Spectrum(0.5f, 0.0f, 0.5f))) {
		std::string params;
		params += "expected: " + to_string(Spectrum(0.5f, 0.0f, 0.5f)) + "\n";
		params += "     got: " + to_string(levels.back().at(0,0));
		puts("");
		info("%s",params.c_str());
		throw Test::error("Mipmap generation didn't approximately average image in last level.");
	}


});


//-------------------------------------------
//check LOD computation in lambertian program:

Test test_a1_task6_lod_simple("a1.task6.lod.simple", []() {

	using L = Programs::Lambertian;

	L::Parameters parameters;

	parameters.local_to_clip = Mat4::I;
	parameters.normal_to_world = Mat4::I;

	Textures::Image test_texture = test_mipmap_texture();
	parameters.image = &test_texture;

	//lighting is uniform (1,1,1) in whole sphere:
	parameters.sun_energy = Spectrum(0.0f, 0.0f, 0.0f);
	parameters.sun_direction = Vec3(0.0f, 0.0f, 1.0f);
	parameters.sky_energy = Spectrum(1.0f, 1.0f, 1.0f);
	parameters.ground_energy = Spectrum(1.0f, 1.0f, 1.0f);
	parameters.sky_direction = Vec3(0.0f, 0.0f, 1.0f);

	std::array< float, 5 > attribs; attribs.fill(0.0f);
	//texture coordinate (0.5f, 0.5f):
	attribs[L::FA_TexCoordU] = 0.5f; attribs[L::FA_TexCoordV] = 0.5f;
	//normal pointing straight up:
	attribs[L::FA_NormalX] = 0.0f; attribs[L::FA_NormalY] = 0.0f; attribs[L::FA_NormalZ] = 1.0f;

	std::array< Vec2, 2 > derivs; derivs.fill(Vec2{0.0f, 0.0f});


	auto expect_lod = [&](std::string const &desc, Vec2 du, Vec2 dv, float min_lod, float max_lod){
		Spectrum out_color;
		float out_opacity;

		derivs[L::FA_TexCoordU] = du;
		derivs[L::FA_TexCoordV] = dv;
		L::shade_fragment(parameters, attribs, derivs, &out_color, &out_opacity);

		float lod = out_color.b;

		if (!(min_lod <= lod && lod <= max_lod)) {
			std::cout << "Note: LOD requires Trilinear to be complete\n";
			std::string params;
			params += "case '" + desc + "':\n";
			params += "  du/dx, du/dy: " + to_string(du) + "\n";
			params += "  dv/dx, dv/dy: " + to_string(dv) + "\n";
			params += "  expected: " + std::to_string(min_lod) + " <= lod <= " + std::to_string(max_lod) + "\n";
			params += "   got lod: " + std::to_string(lod);

			info("%s",params.c_str());

			throw Test::error("Lod outside expected range in '" + desc + "' case.");
		}
	};

	Vec2 px_to_texcoord = Vec2(1.0f / test_texture.image.w, 1.0f / test_texture.image.h);

	expect_lod("zero texels per pixel", px_to_texcoord * Vec2(0.0f, 0.0f), px_to_texcoord * Vec2(0.0f, 0.0f), -0.001f, 0.001f);
	//larger leeway here given how many methods one might come up with.
	//    but should generally be closer to level zero than not!
	expect_lod("one texels per pixel", px_to_texcoord * Vec2(1.0f, 0.0f), px_to_texcoord * Vec2(0.0f, 1.0f), -0.001f, 0.501f);
	//covering two texels, should definitely be into level 1:
	expect_lod("two texels per pixel", px_to_texcoord * Vec2(2.0f, 0.0f), px_to_texcoord * Vec2(0.0f, 2.0f), 0.5f, 1.5f);
	//covering four texels, should be into level 2:
	expect_lod("four texels per pixel", px_to_texcoord * Vec2(4.0f, 0.0f), px_to_texcoord * Vec2(0.0f, 4.0f), 1.5f, 2.5f);

});

