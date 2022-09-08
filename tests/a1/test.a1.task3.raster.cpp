#include "test.h"
#include "rasterizer/pipeline.h"
#include "rasterizer/programs.h"
#include "rasterizer/framebuffer.h"

#include <limits>
#include <iomanip>
#include <algorithm>
#include <iostream>

using FlatPipeline = Pipeline< PrimitiveType::Triangles, Programs::Lambertian, Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Flat >;
using FPClippedVertex = FlatPipeline::ClippedVertex;
using FPFragment = FlatPipeline::Fragment;

[[maybe_unused]]
static std::string to_string(FPClippedVertex const &vert) {
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
[[maybe_unused]]
static std::string to_string(FPFragment const &frag) {
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

static void check_rasterize_triangles(std::string const &desc, std::initializer_list< FlatPipeline::ClippedVertex > const &vertices_, std::initializer_list< FlatPipeline::Fragment > const &expected_) {
	std::vector< FlatPipeline::ClippedVertex > vertices(vertices_);
	std::vector< FlatPipeline::Fragment > expected(expected_);

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
		FlatPipeline::Fragment fragment; //fragment emitted
		uint8_t id; //triangle that it came from
	};
	std::vector< FragId > got;
	uint8_t current_tri = 0;
	auto emit_fragment = [&](FlatPipeline::Fragment const &f) {
		got.emplace_back(FragId{f, current_tri});
	};
	for (uint32_t i = 2; i < vertices.size(); i += 3) {
		current_tri = uint8_t(i/3);
		FlatPipeline::rasterize_triangle(vertices[i-2], vertices[i-1], vertices[i], emit_fragment);
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
			std::function< bool(Vec2, Vec2) > derivative_close = [](Vec2 a, Vec2 b) { return !Test::differs(a,b); };
			//check attributes (approximately) match:
			if (!std::equal(g.attributes.begin(), g.attributes.end(), e.attributes.begin(), e.attributes.end(), attribute_close)) continue;
			//check derivatives (approximately) match:
			if (!std::equal(g.derivatives.begin(), g.derivatives.end(), e.derivatives.begin(), e.derivatives.end(), derivative_close)) continue;

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
			std::cout << "    v[" << i << "]: " << to_string(vertices[i]) << '\n';
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
					std::cout << "    f[" << (&f - &got[0]) << "]: " << to_string(f.fragment) << notes[fi] << '\n';
				}
			}
		}
		std::cout << "  (for a total of " << got.size() << " fragments from " << vertices.size()/3 << " triangles.)\n";
		std::cout << "  expected " << expected.size() << " fragments:\n";
		for (uint32_t i = 0; i < expected.size(); ++i) {
			auto const &f = expected[i];
			std::cout << "    " << to_string(f);
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
// Flat-shaded triangles.

Test test_a1_task3_raster_simple_1px("a1.task3.raster.simple.1px", []() {
	check_rasterize_triangles(
		"triangle inside [2,3]x[1,2] covering (2.5,1.5)",
		{ FPClippedVertex{ Vec3{ 2.1f, 1.1f, 0.5f }, 1.0f, { 1.0f } },
		  FPClippedVertex{ Vec3{ 2.1f, 1.9f, 0.5f }, 1.0f, { 2.0f } },
		  FPClippedVertex{ Vec3{ 2.9f, 1.6f, 0.5f }, 1.0f, { 3.0f } } },
		{ FPFragment{ Vec3{ 2.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } } }
	);
});

Test test_a1_task3_raster_flag_E("a1.task3.raster.flag.E", []() {
	check_rasterize_triangles(
		"east-pointing flag",
		{
			FPClippedVertex{ Vec3{ 0.25f, 0.25f, 0.5f }, 1.0f, { 1.0f } },
			FPClippedVertex{ Vec3{ 2.75f, 1.50f, 0.5f }, 1.0f, { 2.0f } },
			FPClippedVertex{ Vec3{ 0.25f, 2.75f, 0.5f }, 1.0f, { 3.0f } }
		},
		{
			FPFragment{ Vec3{ 0.5f, 0.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 0.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 2.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 0.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
		}
	);
});

Test test_a1_task3_raster_flag_W("a1.task3.raster.flag.W", []() {
	check_rasterize_triangles(
		"west-pointing flag",
		{
			FPClippedVertex{ Vec3{ 0.25f, 1.50f, 0.5f }, 1.0f, { 1.0f } },
			FPClippedVertex{ Vec3{ 2.75f, 0.25f, 0.5f }, 1.0f, { 2.0f } },
			FPClippedVertex{ Vec3{ 2.75f, 2.75f, 0.5f }, 1.0f, { 3.0f } }
		},
		{
			FPFragment{ Vec3{ 2.5f, 0.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 0.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 2.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 2.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
		}
	);
});

Test test_a1_task3_raster_flag_N("a1.task3.raster.flag.N", []() {
	check_rasterize_triangles(
		"north-pointing flag",
		{
			FPClippedVertex{ Vec3{ 0.25f, 0.25f, 0.5f }, 1.0f, { 1.0f } },
			FPClippedVertex{ Vec3{ 2.75f, 0.25f, 0.5f }, 1.0f, { 2.0f } },
			FPClippedVertex{ Vec3{ 1.50f, 2.75f, 0.5f }, 1.0f, { 3.0f } }
		},
		{
			FPFragment{ Vec3{ 0.5f, 0.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 0.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 2.5f, 0.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
		}
	);
});

Test test_a1_task3_raster_flag_S("a1.task3.raster.flag.S", []() {
	check_rasterize_triangles(
		"south-pointing flag",
		{
			FPClippedVertex{ Vec3{ 1.50f, 0.25f, 0.5f }, 1.0f, { 1.0f } },
			FPClippedVertex{ Vec3{ 2.75f, 2.75f, 0.5f }, 1.0f, { 2.0f } },
			FPClippedVertex{ Vec3{ 0.25f, 2.75f, 0.5f }, 1.0f, { 3.0f } }
		},
		{
			FPFragment{ Vec3{ 1.5f, 0.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 0.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 1.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
			FPFragment{ Vec3{ 2.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
		}
	);
});


Test test_a1_task3_raster_thin_2px("a1.task3.raster.thin.2px", []() {
	check_rasterize_triangles(
		"thin triangle exceeding [2,3]x[1,3] but only covering (2.5,1.5) and (2.5,2.5)",
		{ FPClippedVertex{ Vec3{ 1.75f, 0.75f, 0.5f }, 1.0f, { 1.0f } },
		  FPClippedVertex{ Vec3{ 3.25f, 0.75f, 0.5f }, 2.0f, { 2.0f } },
		  FPClippedVertex{ Vec3{ 2.50f, 3.25f, 0.5f }, 3.0f, { 3.0f } } },
		{ FPFragment{ Vec3{ 2.5f, 1.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } },
		  FPFragment{ Vec3{ 2.5f, 2.5f, 0.5f }, {1.0f}, { Vec2{0.0f} } } }
	);
});


