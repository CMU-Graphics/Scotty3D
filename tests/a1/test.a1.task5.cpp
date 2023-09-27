#include "test.h"
#include "rasterizer/pipeline.h"
#include "rasterizer/programs.h"
#include "rasterizer/framebuffer.h"

#include <limits>
#include <iomanip>
#include <algorithm>
#include <iostream>

using ScreenPipeline = Pipeline< PrimitiveType::Triangles, Programs::Lambertian, Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Smooth >;
using SPClippedVertex = ScreenPipeline::ClippedVertex;
using SPFragment = ScreenPipeline::Fragment;

using CorrectPipeline = Pipeline< PrimitiveType::Triangles, Programs::Lambertian, Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Correct >;
using CPClippedVertex = CorrectPipeline::ClippedVertex;
using CPFragment = CorrectPipeline::Fragment;

template< typename P >
[[maybe_unused]]
static std::string to_string(typename P::ClippedVertex const &vert) {
	std::string ret = "{"
		" .fb_position = (" + std::to_string(vert.fb_position.x) + ", " + std::to_string(vert.fb_position.y) + ", " + std::to_string(vert.fb_position.z) + ")"
		", .inv_w = " + std::to_string(vert.inv_w);
	ret += ", .attributes = [";
	for (auto const &a : vert.attributes) {
		if (&a != &vert.attributes[0]) ret += ", ";
		ret += std::to_string(a);
	}
	ret += "] }";
	return ret;
}
template< typename P >
[[maybe_unused]]
static std::string to_string(typename P::Fragment const &frag) {
	std::string ret = "{"
		" .fb_position = (" + std::to_string(frag.fb_position.x) + ", " + std::to_string(frag.fb_position.y) + ", " + std::to_string(frag.fb_position.z) + ")";
	ret += ", .attributes = [";
	for (auto const &a : frag.attributes) {
		if (&a != &frag.attributes[0]) ret += ", ";
		ret += std::to_string(a);
	}
	ret += "]";
	ret += ", .derivatives = [";
	for (auto const &d : frag.derivatives) {
		if (&d != &frag.derivatives[0]) ret += ", ";
		ret += "(" + std::to_string(d.x) + ", " + std::to_string(d.y) + ")";
	}
	ret += "]";
	return ret;
}

enum CheckDerivativesMode : int {
	CheckDerivatives,
	CheckDerivativeSigns
};

template< class P, int check_derivatives = CheckDerivatives >
static void check_rasterize_triangles(std::string const &desc, std::initializer_list< typename P::ClippedVertex > const &vertices_, std::initializer_list< typename P::Fragment > const &expected_) {
	std::vector< typename P::ClippedVertex > vertices(vertices_);
	std::vector< typename P::Fragment > expected(expected_);

	if (vertices.size() % 3 != 0) {
		throw Test::error("Example '" + desc + "' is INVALID because the vertex count (" + std::to_string(vertices.size()) + ") is not divisible by three.");
	}

	for (auto const &v : vertices) {
		if (v.fb_position.x < 0.0f || v.fb_position.x > float(Framebuffer::MaxWidth)
		 || v.fb_position.y < 0.0f || v.fb_position.y > float(Framebuffer::MaxHeight)
		 || v.fb_position.z < 0.0f || v.fb_position.z > 1.0f) {
			throw Test::error("Example '" + desc + "' is INVALID because it includes a vertex with fb_position " + to_string(v.fb_position) + ", which lies outside the largest possible framebuffer [0," + std::to_string(Framebuffer::MaxWidth) + "]x[0," + std::to_string(Framebuffer::MaxHeight) + "]x[0,1].");
		}
	}


	//rasterize triangles:
	struct FragId {
		typename P::Fragment fragment; //fragment emitted
		uint8_t id; //triangle that it came from
	};
	std::vector< FragId > got;
	uint8_t current_tri = 0;
	auto emit_fragment = [&](typename P::Fragment const &f) {
		got.emplace_back(FragId{f, current_tri});
	};
	for (uint32_t i = 2; i < vertices.size(); i += 3) {
		current_tri = uint8_t(i/3);
		P::rasterize_triangle(vertices[i-2], vertices[i-1], vertices[i], emit_fragment);
	}


	//check the results:

	std::vector< bool > matched(expected.size(), false);
	std::vector< std::string > notes(got.size());

	uint32_t matches = 0;
	uint32_t missing = 0;
	uint32_t overlaps = 0;
	uint32_t wrong_data = 0;
	uint32_t wrong_position = 0;

	for (uint32_t gi = 0; gi < got.size(); ++gi) {
		auto const &g = got[gi].fragment;
		std::vector< uint32_t > position_matches;
		uint32_t found_index = -1U;
		bool found_overlap = false;
		for (uint32_t i = 0; i < expected.size(); ++i) {
			auto const &e = expected[i];
			//check for exact position match, since positions are always exactly representable as floating point:
			if (!(g.fb_position.x == e.fb_position.x && g.fb_position.y == e.fb_position.y)) continue;
			position_matches.emplace_back(i);
			//check z and attribs and derivatives (approximately) match:
			if (Test::differs(g.fb_position.z, e.fb_position.z)) continue;
			std::function< bool(float, float) > attribute_close = [](float a, float b) { return !Test::differs(a,b); };
			//check attributes (approximately) match:
			if (!std::equal(g.attributes.begin(), g.attributes.end(), e.attributes.begin(), e.attributes.end(), attribute_close)) continue;
			if constexpr (check_derivatives == CheckDerivativeSigns) {
				//check derivatives match in sign:
				std::function< bool(Vec2, Vec2) > derivatives_same_sign = [](Vec2 a, Vec2 b) { return (a.x > 0.0f) == (b.x > 0.0f) && (a.y > 0.0f) == (b.y > 0.0f); };
				if (!std::equal(g.derivatives.begin(), g.derivatives.end(), e.derivatives.begin(), e.derivatives.end(), derivatives_same_sign)) {
					std::cout << "DERIVATIVES SIGNS DID NOT MATCH\n";
					continue;
				}
			} else {
				static_assert(check_derivatives == CheckDerivatives, "Recognized check mode.");
				//check derivatives (approximately) match:
				std::function< bool(Vec2, Vec2) > derivative_close = [](Vec2 a, Vec2 b) { return !Test::differs(a,b); };
				if (!std::equal(g.derivatives.begin(), g.derivatives.end(), e.derivatives.begin(), e.derivatives.end(), derivative_close)) continue;
			}

			//fragments match!
			if (matched[i]) {
				//but this fragment already matched, so note it but continue looking:
				found_index = i;
				found_overlap = true;
				continue;
			}

			//fragment didn't already match, so mark it and stop looking:
			matched[i] = true;
			found_index = i;
			found_overlap = false;
			break;
		}

		if (found_index != -1U) {
			if (found_overlap == false) {
				//great!
				++matches;
			} else {
				//found, but also had already been found.
				++overlaps;
				notes[gi] = " [OVERLAPPING]";
			}
		} else {
			if (!position_matches.empty()) {
				//not found, but some other fragments with different z or attribs were here.
				++wrong_data;
				notes[gi] = " [WRONG DATA]";
			} else {
				//not found, and no expected fragments were here
				++wrong_position;
				notes[gi] = " [NO MATCH]";
			}
		}
	}

	//which expected fragments were missed?
	for (uint32_t i = 0; i < expected.size(); ++i) {
		if (!matched[i]) {
			++missing;
		}
	}

	//helper to dump all this info if there were errors:
	auto dump_info = [&]() {
		std::cout << '\n';
		for (uint32_t i = 0; i < vertices.size(); ++i) {
			std::cout << "    v[" << i << "]: " << to_string< P >(vertices[i]) << '\n';
		}

		for (uint32_t i = 2; i < vertices.size(); i += 3) {
			uint8_t id = uint8_t(i/3);
			uint32_t count = 0;
			for (auto const &f : got) {
				if (f.id == id) ++count;
			}
			std::cout << "  rasterize_triangle(v[" << i-2 << "],v[" << i-1 << "],v[" << i-2 << "]) emitted " << count << " fragments:\n";
			for (auto const &f : got) {
				uint32_t fi = uint32_t(&f - &got[0]);
				if (f.id == id) {
					std::cout << "    f[" << (&f - &got[0]) << "]: " << to_string< P >(f.fragment) << notes[fi] << '\n';
				}
			}
		}
		std::cout << "  (for a total of " << got.size() << " fragments from " << vertices.size()/3 << " triangles.)\n";
		std::cout << "  expected " << expected.size() << " fragments:\n";
		for (uint32_t i = 0; i < expected.size(); ++i) {
			auto const &f = expected[i];
			std::cout << "    " << to_string< P >(f);
			if (!matched[i]) std::cout << " [MISSING]";
			std::cout << '\n';
		}

		//TODO: rasterized view?

		std::cout << "  " << matches << "/" << expected.size() << " fragments are correct;";
		std::cout << " " << missing << " fragments are missing;";
		std::cout << " " << overlaps << " fragments are overlapped/duplicated;";
		std::cout << " " << wrong_position << " fragments are in unexpected positions;";
		std::cout << " and " << wrong_data << " fragments are in expected positions but have the wrong data.\n";


		std::cout.flush();
	};


	if (missing) {
		dump_info();
		throw Test::error("Example '" + desc + "' had missing fragments");
	}
	if (overlaps) {
		dump_info();
		throw Test::error("Example '" + desc + "' had duplicated fragments");
	}
	if (wrong_position) {
		dump_info();
		throw Test::error("Example '" + desc + "' had unexpected fragments");
	}
	if (wrong_data) {
		dump_info();
		throw Test::error("Example '" + desc + "' had fragments with incorrect data");
	}

	//dump_info(); //DEBUG

};

//--------------------------------------------------
// Screen-space interpolation.

Test test_a1_task5_screen_simple_attribs("a1.task5.screen.simple.attribs", []() {
	//Notice that triangle:
	// (1.0, 0.5) - (3.0, 0.0) - (3.0, 1.0)
	//covers (1.5, 0.5) and (2.5, 0.5)
	// with barycentric coordinates:
	//  [0.75, 0.125, 0.125]  and [0.25, 0.375, 0.375], respectively

	//changes in x change barycentric coords by [-0.5 * dx, 0.25 * dx, 0.25 * dx] (since must go from [1.0,0.0,0.0] to [0.0,0.5,0.5] over 2 units)
	//changes in y change barycentric coords by [0.0, -dy, dy] (since must go from [0.0,1.0,0.0] to [0.0,0.0,1.0] over 1 unit)

	Vec2 attrib0_deriv = Vec2{
		1.0f * -0.5f + 2.0f * 0.25f + 4.0f * 0.25f,
		1.0f *  0.0f + 2.0f * -1.0f + 4.0f * 1.0f
	};

	check_rasterize_triangles< ScreenPipeline >(
		"thin triangle over (1.5,0.5) and (2.5,0.5)",
		{ SPClippedVertex{ Vec3{ 1.0f, 0.5f, 0.5f }, 1.0f, { 1.0f } },
		  SPClippedVertex{ Vec3{ 3.0f, 0.0f, 0.5f }, 1.0f, { 2.0f } },
		  SPClippedVertex{ Vec3{ 3.0f, 1.0f, 0.5f }, 1.0f, { 4.0f } } },
		{ SPFragment{ Vec3{ 1.5f, 0.5f, 0.5f }, {1.0f * 0.75f + 2.0f * 0.125f + 4.0f * 0.125f}, { attrib0_deriv, Vec2{0.0f} } },
		  SPFragment{ Vec3{ 2.5f, 0.5f, 0.5f }, {1.0f * 0.25f + 2.0f * 0.375f + 4.0f * 0.375f}, { attrib0_deriv, Vec2{0.0f} } }}
	);
});

Test test_a1_task5_screen_simple_depth("a1.task5.screen.simple.depth", []() {
	check_rasterize_triangles< ScreenPipeline >(
		"thin triangle over (1.5,0.5) and (2.5,0.5)",
		{ SPClippedVertex{ Vec3{ 1.0f, 0.5f, 0.1f }, 1.0f, { 3.0f } },
		  SPClippedVertex{ Vec3{ 3.0f, 0.0f, 0.4f }, 1.0f, { 3.0f } },
		  SPClippedVertex{ Vec3{ 3.0f, 1.0f, 0.8f }, 1.0f, { 3.0f } } },
		{ SPFragment{ Vec3{ 1.5f, 0.5f, 0.1f * 0.75f + 0.4f * 0.125f + 0.8f * 0.125f }, {3.0f}, { Vec2{0.0f} } }, 
		  SPFragment{ Vec3{ 2.5f, 0.5f, 0.1f * 0.25f + 0.4f * 0.375f + 0.8f * 0.375f }, {3.0f}, { Vec2{0.0f} } }}
	);
});


//--------------------------------------------------
// Perspective-correct interpolation.


//uniform-inverse-w triangles should look the same as screen-space triangles:
Test test_a1_task5_correct_simple_attribs("a1.task5.correct.simple.attribs", []() {
	//Note: same triangle as above

	Vec2 attrib0_deriv = Vec2{
		1.0f * -0.5f + 2.0f * 0.25f + 4.0f * 0.25f,
		1.0f *  0.0f + 2.0f * -1.0f + 4.0f * 1.0f
	};
	check_rasterize_triangles< CorrectPipeline >(
		"thin triangle over (1.5,0.5) and (2.5,0.5)",
		{ CPClippedVertex{ Vec3{ 1.0f, 0.5f, 0.5f }, 1.0f, { 1.0f } },
		  CPClippedVertex{ Vec3{ 3.0f, 0.0f, 0.5f }, 1.0f, { 2.0f } },
		  CPClippedVertex{ Vec3{ 3.0f, 1.0f, 0.5f }, 1.0f, { 4.0f } } },
		{ CPFragment{ Vec3{ 1.5f, 0.5f, 0.5f }, {1.0f * 0.75f + 2.0f * 0.125f + 4.0f * 0.125f}, { attrib0_deriv, Vec2{0.0f} } },
		  CPFragment{ Vec3{ 2.5f, 0.5f, 0.5f }, {1.0f * 0.25f + 2.0f * 0.375f + 4.0f * 0.375f}, { attrib0_deriv, Vec2{0.0f} } }}
	);
});

Test test_a1_task5_correct_simple_depth("a1.task5.correct.simple.depth", []() {
	check_rasterize_triangles< CorrectPipeline >(
		"thin triangle over (1.5,0.5) and (2.5,0.5)",
		{ CPClippedVertex{ Vec3{ 1.0f, 0.5f, 0.1f }, 1.0f, { 3.0f } },
		  CPClippedVertex{ Vec3{ 3.0f, 0.0f, 0.4f }, 1.0f, { 3.0f } },
		  CPClippedVertex{ Vec3{ 3.0f, 1.0f, 0.8f }, 1.0f, { 3.0f } } },
		{ CPFragment{ Vec3{ 1.5f, 0.5f, 0.1f * 0.75f + 0.4f * 0.125f + 0.8f * 0.125f }, {3.0f}, { Vec2{0.0f} } }, 
		  CPFragment{ Vec3{ 2.5f, 0.5f, 0.1f * 0.25f + 0.4f * 0.375f + 0.8f * 0.375f }, {3.0f}, { Vec2{0.0f} } }}
	);
});

Test test_a1_task5_correct_persp("a1.task5.correct.persp", []() {
	//adding invw values:

	//(expected attributes computed by hand using perspective correct interpolation formula)

	//NOTE: many possible derivative computation methods -- will only check sign
	Vec2 attrib0_deriv = Vec2{ 1.0f, 1.0f };
	check_rasterize_triangles< CorrectPipeline, CheckDerivativeSigns >(
		"thin triangle over (1.5,0.5) and (2.5,0.5)",
		{ CPClippedVertex{ Vec3{ 1.0f, 0.5f, 0.5f }, 1.0f, { 1.0f } },
		  CPClippedVertex{ Vec3{ 3.0f, 0.0f, 0.5f }, 2.0f, { 2.0f } },
		  CPClippedVertex{ Vec3{ 3.0f, 1.0f, 0.5f }, 2.0f, { 4.0f } } },
		{ CPFragment{ Vec3{ 1.5f, 0.5f, 0.5f }, {1.8f}, { attrib0_deriv, Vec2{0.0f} } },
		  CPFragment{ Vec3{ 2.5f, 0.5f, 0.5f }, {2.714285f}, { attrib0_deriv, Vec2{0.0f} } }}
	);
});

