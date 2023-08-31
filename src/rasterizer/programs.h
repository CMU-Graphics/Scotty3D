#pragma once
// clang-format off
/*
 * Various Program (vertex + shader) programs for use with software rasterizer pipeline.
 *
 */

#include "../lib/mathlib.h"
#include "../scene/texture.h"

namespace Programs {

struct Lambertian {
	struct Parameters {
		// transformations (used in shade_vertex):
		Mat4 local_to_clip;
		Mat4 normal_to_world;

		// textures (used in shade_fragment):
		Textures::Image const* image = nullptr;
		// NOTE: texture is a pointer because copying textures around is slow.
		//   	 shade_fragment will crash if it doesn't point to a valid texture!

		// opacity is set for the whole object (since textures don't have alpha):
		float opacity = 1.0f;

		// light data (used in shade_fragment):

		// a distant directional light:
		Spectrum sun_energy;
		Vec3 sun_direction;

		// and a sphere that is 'sky_energy' above the equator and 'ground_energy' below:
		Spectrum sky_energy;
		Spectrum ground_energy;
		Vec3 sky_direction;
	};

	// vertex attributes layout:
	enum {
		VA_PositionX,
		VA_PositionY,
		VA_PositionZ,
		VA_NormalX,
		VA_NormalY,
		VA_NormalZ,
		VA_TexCoordU,
		VA_TexCoordV,
		VA
	};
	// fragment attribute layout:

	enum { 
		FA_TexCoordU, 
		FA_TexCoordV, 
		FA_NormalX, 
		FA_NormalY, 
		FA_NormalZ, 
		FA
	};
	// request derivatives for first two attributes (the texture coordinates):
	enum { 
		FD = 2 
	};


	static void
	shade_vertex(Parameters const& parameters, std::array<float, VA> const& va, // vertex attributes
	             Vec4* clip_position_, // output clip position (must be non-null)
	             std::array<float, FA>* fa_ // output attributes to interpolate to fragments (must be non-null)
	) {

		auto& clip_position = *clip_position_;
		auto& fa = *fa_;

		// make local names for vertex attributes:
		Vec3 va_position{va[VA_PositionX], va[VA_PositionY], va[VA_PositionZ]};
		Vec3 va_normal{va[VA_NormalX], va[VA_NormalY], va[VA_NormalZ]};
		Vec2 va_texcoord{va[VA_TexCoordU], va[VA_TexCoordV]};

		// compute clip position:
		clip_position = parameters.local_to_clip * Vec4(va_position, 1.0f);

		// compute fragment attributes:
		Vec3 fa_normal = parameters.normal_to_world * va_normal;
		Vec2 fa_texcoord = va_texcoord;

		// store fragment attributes to output:
		fa[FA_TexCoordU] = fa_texcoord.x;
		fa[FA_TexCoordV] = fa_texcoord.y;
		fa[FA_NormalX] = fa_normal.x;
		fa[FA_NormalY] = fa_normal.y;
		fa[FA_NormalZ] = fa_normal.z;
	}

	static void shade_fragment(Parameters const& parameters,
	                           std::array<float, FA> const& fa, // interpolated fragment attributes
	                           std::array<Vec2, FD> const& fd,  // fragment attribute derivatives
	                           Spectrum* color_,                // output color (must be non-null)
	                           float* opacity_                  // output opacity (must be non-null)
	) {
		auto& color = *color_;
		auto& opacity = *opacity_;

		// make local names for fragment attributes and derivatives:
		Vec2 fa_texcoord{fa[FA_TexCoordU], fa[FA_TexCoordV]};
		Vec3 fa_normal{fa[FA_NormalX], fa[FA_NormalY], fa[FA_NormalZ]};

		// make local names for fragment attribute derivatives:
		Vec2 fdx_texcoord{fd[FA_TexCoordU].x, fd[FA_TexCoordV].x};
		Vec2 fdy_texcoord{fd[FA_TexCoordU].y, fd[FA_TexCoordV].y};

		// size of texture image:
		[[maybe_unused]] Vec2 wh =
			Vec2(float(parameters.image->image.w), float(parameters.image->image.h));

		//-----
		// A1T6: lod
		// TODO: compute mip-map level based on size of sample as estimated from derivatives:

		// Read section 3.8.11 of glspec33.core.pdf to understand how to compute this level
		//  --> 'lod' is \lambda_base from equation (3.17)
		// reading onward, you will discover that \rho can be computed in a number of ways
		//  it is up to you to select one that makes sense in this context

		float lod = 0.0f; //<-- replace this line
		//-----

		Vec3 normal = fa_normal.unit();

		Spectrum light =
			// sun contribution:
			parameters.sun_energy * std::max(dot(parameters.sun_direction, normal), 0.0f)
			// sky contribution:
			+ (parameters.sky_energy - parameters.ground_energy) *
				  (0.5f * dot(parameters.sky_direction, normal) + 0.5f) +
			parameters.ground_energy;

		color = parameters.image->evaluate(fa_texcoord, lod) * light;
		opacity = parameters.opacity;
	}
};

// The 'Copy' shader copies everything from vertex attributes:
// 		(useful for testing!)
struct Copy {
	struct Parameters {};

	// vertex attributes layout:
	enum {
		VA_PositionX,
		VA_PositionY,
		VA_PositionZ,
		VA_PositionW,
		VA_ColorR,
		VA_ColorG,
		VA_ColorB,
		VA_ColorA,
		VA
	};
	// fragment attribute layout:
	enum { 
		FA_ColorR, 
		FA_ColorG, 
		FA_ColorB, 
		FA_ColorA, 
		FA 
	};
	// request derivatives for first two attributes (ColorR, ColorG):
	enum { 
		FD = 2 
	};

	static void
	shade_vertex(Parameters const& parameters, std::array<float, VA> const& va, // vertex attributes
	             Vec4* clip_position_, // output clip position (must be non-null)
	             std::array<float, FA>* fa_ // output attributes to interpolate to fragments (must be non-null)
	) {
		auto& clip_position = *clip_position_;
		auto& fa = *fa_;

		// copy clip position from input (x,y,z,w):
		clip_position =
			Vec4(va[VA_PositionX], va[VA_PositionY], va[VA_PositionZ], va[VA_PositionW]);

		// copy color to fragment attributes:
		fa[FA_ColorR] = va[VA_ColorR];
		fa[FA_ColorG] = va[VA_ColorG];
		fa[FA_ColorB] = va[VA_ColorB];
		fa[FA_ColorA] = va[VA_ColorA];
	}

	static void shade_fragment(
		Parameters const& parameters,
		// TODO: should we have -> Vec3 const &fb_position, //fragment position in the framebuffer
		std::array<float, FA> const& fa, // interpolated fragment attributes
		std::array<Vec2, FD> const& fd,  // fragment attribute derivatives
		Spectrum* color_,                // output color (must be non-null)
		float* opacity_                  // output opacity (must be non-null)
	) {
		auto& color = *color_;
		auto& opacity = *opacity_;

		color = Spectrum(fa[FA_ColorR], fa[FA_ColorG], fa[FA_ColorB]);
		opacity = fa[FA_ColorA];
	}
};

} // namespace Programs
