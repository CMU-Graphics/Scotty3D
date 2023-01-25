#include "test.h"


//Actually include the *definitions* (not just the declarations):
#include "rasterizer/pipeline.cpp"
// This is needed because the test code below instantiates the
// Pipeline< > template with some parameters that *aren't* explicitly
// compiled already (see the bottom of pipeline.cpp).


//helper that makes a triangle that covers (1.5, 1.5) on a 2x2 framebuffer, at requested depth and color:
// NOTE: assumes Pipeline's Program is 'Copy'
template< typename P >
static std::vector< typename P::Vertex > test_triangle(float depth, Spectrum color, float opacity) {
	using PVertex = typename P::Vertex;

	std::vector< PVertex > vertices;
	vertices.emplace_back( PVertex{ std::array< float, 8 >{ 0.25f, 0.25f, depth, 1.0f,  color.r, color.g, color.b, opacity } } );
	vertices.emplace_back( PVertex{ std::array< float, 8 >{ 0.75f, 0.50f, depth, 1.0f,  color.r, color.g, color.b, opacity } } );
	vertices.emplace_back( PVertex{ std::array< float, 8 >{ 0.50f, 0.75f, depth, 1.0f,  color.r, color.g, color.b, opacity } } );
	return vertices;
}

//helper that makes a 2x2 framebuffer filled with a specific 'blank' color:
static const Spectrum BlankColor(0.31415926f, 0.0f, 0.31415926f);
static const float BlankDepth = 0.31415926f;
static Framebuffer test_fb() {
	//id 1 is guaranteed to be "single sample at pixel center":
	static SamplePattern const *center = SamplePattern::from_id(1);
	assert(center && center->centers_and_weights.size() == 1 && center->centers_and_weights[0] == Vec3(0.5f, 0.5f, 1.0f) && "center sample pattern exists and is as expected");

	Framebuffer fb(2,2,*center);
	fb.colors.assign(fb.colors.size(), BlankColor);
	fb.depths.assign(fb.depths.size(), BlankDepth);

	return fb;
}


//helper that gets fragment depth corresponding to vertex depth (given our standard triangle):
// NOTE: assumes Pipeline's Program is 'Copy'
[[maybe_unused]]
static void fragment_depth_to_interpolated_and_vertex_depth(float wanted_depth, float *interpolated_depth_, float *vertex_depth_) {
	assert(interpolated_depth_);
	assert(vertex_depth_);

	//invert the clip -> framebuffer mapping (fb.z = clip.z * 0.5 + 0.5):
	*vertex_depth_ = (wanted_depth - 0.5f) * 2.0f;
	

	//figure out what interpolation does to the value:
	using P = Pipeline< PrimitiveType::Triangles, Programs::Copy, Pipeline_Blend_Replace | Pipeline_Depth_Always | Pipeline_Interp_Flat >;
	Framebuffer fb = test_fb();
	P::run(test_triangle< P >(*vertex_depth_, BlankColor, 1.0f), Programs::Copy::Parameters(), &fb);

	*interpolated_depth_ = fb.depth_at(1,1,0);
}

//see what opacity / color gets written back:
template< int flags >
static void check_fragment_writeback(
	std::string const &desc,
	float vert_depth, Spectrum frag_color, float frag_opacity,
	float fb_depth, Spectrum fb_color,
	float out_depth, Spectrum out_color) {

	using P = Pipeline< PrimitiveType::Triangles, Programs::Copy, (flags & ~PipelineMask_Interp) | Pipeline_Interp_Flat >;
	using PVertex = typename P::Vertex;

	//---------------------
	// set up framebuffer:
	Framebuffer fb = test_fb();

	//write specified depth at pixel (1,1):
	fb.color_at(1,1,0) = fb_color;
	fb.depth_at(1,1,0) = fb_depth;

	//---------------------
	// set up vertices:
	std::vector< PVertex > triangle = test_triangle< P >(vert_depth, frag_color, frag_opacity);

	//---------------------
	//run the pipeline:
	P::run(triangle, Programs::Copy::Parameters(), &fb);

	//---------------------
	//check results:

	//only upper-right pixel was written:
	if (fb.color_at(0,0,0) != BlankColor || fb.color_at(1,0,0) != BlankColor || fb.color_at(0,1,0) != BlankColor) throw Test::error("Pixel color other than upper right was written when running pipeline.");
	if (fb.depth_at(0,0,0) != BlankDepth || fb.depth_at(1,0,0) != BlankDepth || fb.depth_at(0,1,0) != BlankDepth) throw Test::error("Pixel color other than upper right was written when running pipeline.");

	//check that output matches expected:
	if (Test::differs(fb.color_at(1,1,0), out_color) || fb.depth_at(1,1,0) != out_depth) {
		std::string params;
		params += "    vert_depth: " + std::to_string(vert_depth) + " (maps to appox " + std::to_string(vert_depth * 0.5f + 0.5f) + ")\n";
		params += "      fb_depth: " + std::to_string(fb_depth) + "\n";
		params += "      expected: " + std::to_string(out_depth) + "\n";
		params += "           got: " + std::to_string(fb.depth_at(1,1,0));
		if (fb.depth_at(1,1,0) != out_depth) params += "  (DOES NOT MATCH)\n";
		else params += "  (matches)\n";
		params += "    frag_color: " + to_string(frag_color) + "\n";
		params += "      fb_color: " + to_string(fb_color) + "\n";
		params += "      expected: " + to_string(out_color) + "\n";
		params += "           got: " + to_string(fb.color_at(1,1,0)) + "\n";
		if (Test::differs(fb.color_at(1,1,0), out_color)) params += "  (DOES NOT MATCH)\n";
		else params += "  (matches)\n";

		puts("");
		info("Case '%s':\n%s",desc.c_str(), params.c_str());

		throw Test::error("Writing '" + desc + "' got color or depth that does not match expected.");
	}
}

//-----------------------------------------------
//always + replace => fragment always written:

Test test_a1_task4_depth_always_25_50("a1.task4.depth.always.25_50", []() {
	float interpolated_depth, vertex_depth;
	fragment_depth_to_interpolated_and_vertex_depth(0.25f, &interpolated_depth, &vertex_depth);
	if (Test::differs(interpolated_depth, 0.25f)) throw Test::error("triangle depth interpolation not working as expected, can't perform this test.");

	check_fragment_writeback< Pipeline_Blend_Replace | Pipeline_Depth_Always >(
		"d0.25 (always) d0.5",
		vertex_depth, Spectrum(0.75f, 0.50f, 0.25f), 0.5f, //fragment
		0.50f, Spectrum(0.1f, 0.2f, 0.4f), //fb
		interpolated_depth, Spectrum(0.75f, 0.50f, 0.25f) //out
	);
});



//-----------------------------------------------
//less + replace => fragment only when less:

Test test_a1_task4_depth_less_25_50("a1.task4.depth.less.25_50", []() {
	float interpolated_depth, vertex_depth;
	fragment_depth_to_interpolated_and_vertex_depth(0.25f, &interpolated_depth, &vertex_depth);
	if (Test::differs(interpolated_depth, 0.25f)) throw Test::error("triangle depth interpolation not working as expected, can't perform this test.");

	check_fragment_writeback< Pipeline_Blend_Replace | Pipeline_Depth_Less >(
		"d0.25 (less) d0.5",
		vertex_depth, Spectrum(0.75f, 0.50f, 0.25f), 0.5f, //fragment
		0.50f, Spectrum(0.1f, 0.2f, 0.4f), //fb
		interpolated_depth, Spectrum(0.75f, 0.50f, 0.25f) //out
	);
});

Test test_a1_task4_depth_less_75_50("a1.task4.depth.less.75_50", []() {
	float interpolated_depth, vertex_depth;
	fragment_depth_to_interpolated_and_vertex_depth(0.75f, &interpolated_depth, &vertex_depth);
	if (Test::differs(interpolated_depth, 0.75f)) throw Test::error("triangle depth interpolation not working as expected, can't perform this test.");

	check_fragment_writeback< Pipeline_Blend_Replace | Pipeline_Depth_Less >(
		"d0.75 (less) d0.5",
		vertex_depth, Spectrum(0.75f, 0.5f, 0.25f), 0.5f, //fragment
		0.50f, Spectrum(0.1f, 0.2f, 0.4f), //fb
		0.50f, Spectrum(0.1f, 0.2f, 0.4f) //out
	);
});



//-----------------------------------------------
//always + add => sum color (modulated by opacity):

Test test_a1_task4_blend_add_1("a1.task4.blend.add.1", []() {
	float interpolated_depth, vertex_depth;
	fragment_depth_to_interpolated_and_vertex_depth(0.5f, &interpolated_depth, &vertex_depth);
	if (Test::differs(interpolated_depth, 0.5f)) throw Test::error("triangle depth interpolation not working as expected, can't perform this test.");

	check_fragment_writeback< Pipeline_Blend_Add | Pipeline_Depth_Always >(
		"[0.1,0.2,0.3] opacity 1 + [0.4, 0.5, 0.6]",
		vertex_depth, Spectrum(0.1f, 0.2f, 0.3f), 1.0f, //fragment
		1.0f, Spectrum(0.4f, 0.5f, 0.6f), //fb
		interpolated_depth, Spectrum(0.5f, 0.7f, 0.9f) //out
	);
});



//-----------------------------------------------
//always + over => standard over blending:

Test test_a1_task4_blend_over_1("a1.task4.blend.over.1", []() {
	float interpolated_depth, vertex_depth;
	fragment_depth_to_interpolated_and_vertex_depth(0.5f, &interpolated_depth, &vertex_depth);
	if (Test::differs(interpolated_depth, 0.5f)) throw Test::error("triangle depth interpolation not working as expected, can't perform this test.");

	//full opacity blend over replaces:
	check_fragment_writeback< Pipeline_Blend_Over | Pipeline_Depth_Always >(
		"[0.1,0.2,0.3] opacity 1 over [0.4, 0.5, 0.6]",
		vertex_depth, Spectrum(0.1f, 0.2f, 0.3f), 1.0f, //fragment
		1.0f, Spectrum(0.4f, 0.5f, 0.6f), //fb
		interpolated_depth, Spectrum(0.1f, 0.2f, 0.3f) //out
	);
});



