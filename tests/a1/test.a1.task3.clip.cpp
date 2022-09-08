#include "test.h"
#include "rasterizer/pipeline.h"
#include "rasterizer/programs.h"

#include <limits>
#include <iomanip>
#include <algorithm>
#include <iostream>

using TestPipeline = Pipeline< PrimitiveType::Triangles, Programs::Lambertian, Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Flat >;

void check_clip_triangle(std::string const &desc, std::initializer_list< Vec4 > const &verts, std::initializer_list< Vec4 > const &expected_) {

	std::vector< Vec4 > expected(expected_);

	//set up triangle from init:
	assert(verts.size() == 3);
	std::vector< TestPipeline::ShadedVertex > triangle;
	uint32_t idx = 1;
	for (Vec4 const &vert : verts) {
		TestPipeline::ShadedVertex sv;
		sv.clip_position = vert;
		sv.attributes.fill(float(idx));
		triangle.emplace_back(sv);
		++idx;
	}

	//run clip_triangle:
	std::vector< Vec4 > got;
	got.reserve(6);
	auto emit_vertex = [&](TestPipeline::ShadedVertex const &sv) {
		got.emplace_back(sv.clip_position);
	};
	TestPipeline::clip_triangle(triangle[0], triangle[1], triangle[2], emit_vertex);

	auto dump_info = [&]() {
		std::cout << '\n';
		std::cout << "    va: " << triangle[0].clip_position << '\n';
		std::cout << "    vb: " << triangle[1].clip_position << '\n';
		std::cout << "    vc: " << triangle[2].clip_position << '\n';
		std::cout << "  clip_triangle(va,vb,vc) emitted " << float(got.size()) / 3.0f << " triangles:\n";
		for (auto const &g : got) {
			std::cout << "    " << g << '\n';
		}
		std::cout << "  expected triangulation of:\n";
		for (auto const &e : expected) {
			std::cout << "    " << e << '\n';
		}
		std::cout.flush();
	};

	if (got.size() % 3 != 0) {
		dump_info();
		throw Test::error("Example '" + desc + "' emitted a partial triangle (had " + std::to_string(got.size()) + " vertices).");
	}

	{ //check triangle counts
		uint32_t got_triangles = static_cast<uint32_t>(got.size()) / 3;
		uint32_t expected_triangles = std::max(0, int32_t(expected.size()) - 2);
		if (got_triangles != expected_triangles) {
			dump_info();
			throw Test::error("Example '" + desc + "' emitted " + std::to_string(got_triangles) + " triangles, was expecting " + std::to_string(expected_triangles) + " triangles.");
		}
	}

	//figure out which expected vertices each 'got' vertex maps to:
	std::vector< uint32_t > index(got.size(), -1U);
	{
		std::vector< float > distance(got.size(), std::numeric_limits< float >::infinity());
		uint32_t vi = 0;
		for (Vec4 v : expected) {
			for (uint32_t g = 0; g < got.size(); ++g) {
				float test = (got[g] - v).norm_squared();
				if (test < distance[g]) {
					distance[g] = test;
					index[g] = vi;
				}
			}
			++vi;
		}

		for (uint32_t g = 0; g < got.size(); ++g) {
			if (distance[g] > Test::differs_eps * Test::differs_eps) {
				dump_info();
				throw Test::error("Example '" + desc + "' emitted vertex " + to_string(got[g]) + " which is far from all expected vertices.");
			}
		}
	}

	//extract perimeter of got:
	std::vector< bool > on_perimeter( expected.size() * expected.size(), false );
	auto toggle = [&](uint32_t a, uint32_t b) {
		if (on_perimeter[ b * expected.size() + a ]) {
			//if b-a exists, cancel it out:
			on_perimeter[ b * expected.size() + a ] = false;
		} else {
			//add in a-b:
			if (on_perimeter[ a * expected.size() + b ]) {
				dump_info();
				throw Test::error("Example '" + desc + "' mentions edge " + to_string(expected[a]) + "-" + to_string(expected[b]) + " twice.");
			}
			on_perimeter[ a * expected.size() + b ] = true;
		}
	};
	for (uint32_t i = 2; i < got.size(); i += 3) {
		toggle(index[i-2], index[i-1]);
		toggle(index[i-1], index[i  ]);
		toggle(index[i  ], index[i-2]);
	}

	//check that perimeter is exactly what we want:
	for (uint32_t a = 0; a < expected.size(); ++a) {
		for (uint32_t b = 0; b < expected.size(); ++b) {
			if ((a + 1) % expected.size() == b) {
				//wanted perimeter edge
				if (!on_perimeter[a * expected.size() + b]) {
					dump_info();
					throw Test::error("Example '" + desc + "' doesn't have expected edge " + to_string(expected[a]) + "-" + to_string(expected[b]) + ".");
				}
			} else {
				//unwanted edge
				if (on_perimeter[a * expected.size() + b]) {
					dump_info();
					throw Test::error("Example '" + desc + "' has unexpected edge " + to_string(expected[a]) + "-" + to_string(expected[b]) + ".");
				}
			}
		}
	}

};

//--------------------------------------------------
// Clipping.

Test test_a1_task3_clip_simple_w1("a1.task3.clip.simple.w1", []() {
	check_clip_triangle(
		"triangle fully inside clip volume with w=1",
		{ Vec4(0.5f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.5f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 0.5f, 1.0f) },
		{ Vec4(0.5f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.5f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 0.5f, 1.0f) }
	);
});


Test test_a1_task3_clip_simple_outside_x("a1.task3.clip.simple.outside.x", []() {
	check_clip_triangle(
		"triangle outside clip volume along +x",
		{ Vec4(2.5f, 0.0f, 0.0f, 1.0f), Vec4(2.0f, 0.5f, 0.0f, 1.0f), Vec4(2.0f, 0.0f, 0.5f, 1.0f) },
		{ }
	);
});


Test test_a1_task3_clip_simple_outside_w_1("a1.task3.clip.simple.outside.w-1", []() {
	check_clip_triangle(
		"triangle outside clip volume because w is -1",
		{ Vec4(0.5f, 0.0f, 0.0f,-1.0f), Vec4(0.0f, 0.5f, 0.0f,-1.0f), Vec4(0.0f, 0.0f, 0.5f,-1.0f) },
		{ }
	);
});


