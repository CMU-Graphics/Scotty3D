#pragma once
// clang-format off
/*
 * Implementation of a software rasterization pipeline, inspired by the OpenGL 3.3 pipeline.
 *
 */
#include <array>
#include <functional>

#include "../lib/spectrum.h"
#include "../lib/vec2.h"
#include "../lib/vec3.h"
#include "../lib/vec4.h"

struct Framebuffer;

// A `Pipeline` can rasterize two primitive types:
enum class PrimitiveType {
	Lines,    // interpret (vertices[2i], vertices[2i+1]) as a line
	Triangles // interpret (vertices[3i], vertices[3i+1], vertices[3i+2]) as a triangle
};

//Other behavior is captured by a set of flags:
enum PipelineFlags : uint32_t {
	Pipeline_DepthWriteDisableBit = 0x8000, //if 1, depth buffer writes are disabled

	Pipeline_ColorWriteDisableBit = 0x4000, //if 1, color buffer writes are disabled

	Pipeline_Blend_Replace  = 0x0, //incoming fragment color replaces framebuffer color
	Pipeline_Blend_Add      = 0x1, //incoming fragment color sums with framebuffer color
	Pipeline_Blend_Over     = 0x2, //incoming fragment color is 'over blended' using opacity

	Pipeline_Depth_Always   = 0x00, //depth test always passes
	Pipeline_Depth_Never    = 0x10, //depth test never passes (not super useful)
	Pipeline_Depth_Less     = 0x20, //depth test passes if fragment depth is less than stored

	Pipeline_Interp_Flat    = 0x000, //attributes are copied from the first vertex
	Pipeline_Interp_Smooth  = 0x100, //attributes are interpolated linearly (smoothly) in screen space
	Pipeline_Interp_Correct = 0x200, //attributes are interpolated perspective-correctly

	//used when reading flags:
	PipelineMask_Blend      = 0x000f, //low four bits for blending function
	PipelineMask_Depth      = 0x00f0, //next four bits for depth function
	PipelineMask_Interp     = 0x0f00, //next four bits for interpolation mode
};

//A Pipeline processes vertices (fixed-length packets of opaque attributes):
template<uint32_t VA>
struct Vertex {
	std::array< float, VA > attributes; //attributes to pass to Program::shade_vertex
};

// It runs a vertex shader on them to produce shaded vertices that have a
// position and homogeneous coordinates and attributes for the fragment shader:
template<uint32_t FA>
struct ShadedVertex {
	Vec4 clip_position; //position in (homogeneous) clip coordinates
	std::array< float, FA > attributes; //attributes to pass to fragment program
};

// These vertices are assembled into primitives, clipped (possibly producing more primitives),
// divided by w, and passed through a viewport transform to compute positions in the framebuffer,
// resulting in clipped vertices:
template<uint32_t FA>
struct ClippedVertex {
	Vec3 fb_position; //position in "viewport" coordinates ([0,fb.width]x[0,fb.height]x[0,1])
	float inv_w; // 1/w -- needed for perspective-correct interpolation
	std::array< float, FA > attributes; //attributes to pass to fragment program
};

// Clipped vertices are rasterized to create fragments:
template<uint32_t FA, uint32_t FD>
struct Fragment {
	Vec3 fb_position; //position in "viewport" coordinates
	std::array< float, FA > attributes; //attributes to pass to fragment program
	std::array< Vec2, FD > derivatives; //derivatives of first FD attributes w.r.t. fb_position.x and fb_position.y
};

// And fragments are passed to a fragment program to create shaded fragments:
struct ShadedFragment {
	Vec3 fb_position; // position in "viewport" coordinates
	Spectrum color;
	float opacity;
};

//A Pipeline depends on the following configuration:
template<
	PrimitiveType primitive_type, //primitive type (how primitives are assembled for rasterization)
	class Program, //the vertex and fragment programs
	uint32_t flags //flags to pass
	>
struct Pipeline {

	// some constants from the Program for convenience:
	enum { VA = Program::VA }; // vertex attribute count
	enum { FA = Program::FA }; // fragment attribute count
	enum { FD = Program::FD }; // fragment attribute derivative count

	static_assert(uint32_t(FD) <= uint32_t(FA),
	              "Program requests no more derivatives than attributes.");

	// When run, the pipeline...

	//(1) starts with an array of Vertices:
	using Vertex = ::Vertex<VA>;

	//(2) transforms these vertices via Program::shade_vertex to produce ShadedVertices:
	using ShadedVertex = ::ShadedVertex<FA>;

	// helper for clip functions:
	//  returns (b - a) * t + a
	static ShadedVertex lerp(ShadedVertex const& a, ShadedVertex const& b, float t);

	//(3) assembles these vertices into primitives of type primitive_type
	//(4) clips the primitives (possibly producing more/fewer output primitives)
	//    uses one of these helpers, depending on the primitive type:
	static void clip_line(
		ShadedVertex const &a, ShadedVertex const &b, //input line (a,b)
		std::function< void(ShadedVertex const &) > const &emit_vertex //called with vertices of clipped line (if non-empty)
	);
	static void clip_triangle(
		ShadedVertex const &a, ShadedVertex const &b, ShadedVertex const &c, //input triangle (a,b,c)
		std::function< void(ShadedVertex const &) > const &emit_vertex //called with vertices of clipped triangle(s)
	);

	//(5) divides by w and scales to compute positions in the framebuffer:
	using ClippedVertex = ::ClippedVertex<FA>;

	//(6) rasterizes the primitives to produce Fragments:
	using Fragment = ::Fragment<FA, FD>;

	//rasterization uses one of these helper functions, depending on primitive type:
	static void rasterize_line(
		ClippedVertex const &a, ClippedVertex const &b, //line (a,b)
		std::function< void(Fragment const &) > const &emit_fragment //call with every fragment covered by the line
	);
	static void rasterize_triangle(
		ClippedVertex const &a, ClippedVertex const &b, ClippedVertex const &c, //triangle (a,b,c)
		std::function< void(Fragment const &) > const &emit_fragment //call with every fragment covered by the triangle
	);

	//(7) tests fragment depths vs depth buffer (based on flags)

	//(8) transforms fragments via Program::shade_fragment() to produce a color and opacity, stored
	//	  in a ShadedFragment:
	using ShadedFragment = ::ShadedFragment;

	//(9) writes color and/or depth to framebuffer (based on flags)

	// The "run" function wraps the above steps:
	// 		vertices: list of vertices to rasterize
	//  	parameters: global parameters for vertex and fragment programs
	//  	framebuffer (must not be null): framebuffer to write results into
	static void run(std::vector<Vertex> const& vertices,
	                typename Program::Parameters const& parameters, Framebuffer* framebuffer);
};
