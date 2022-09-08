
#pragma once

#include "../lib/mathlib.h"
#include "../platform/gl.h"

// NOTE: renamed util/camera to 3D_Viewer; it will only be used for the gui.
// these cameras will be used for the scene / renderers [path tracer]

class Camera {
public:
	template< typename Sampler >
	std::pair<Ray, float> generate_ray(uint32_t px, uint32_t py) {

		// TODO (PathTracer): Task 1
		// compute the position of the input sensor sample coordinate on the
		// canonical sensor plane one unit away from the pinhole.
		// Tip: Compute the ray direction in camera space and use
		// the camera transform to transform it back into world space.

    	return {Ray(), 0.0f};
	}
	GL::Lines to_gl() const;
	//projection matrix for camera looking down -z axis with y up and x right.
	// near plane maps to -1, far plane to 1.
	Mat4 projection() const;

	float vertical_fov = 60.0f;    // Vertical field of view (degrees)
	float aspect_ratio = 1.77778f; // width / height
	float near_plane = 0.1f;

	//'film_' parameters specify how the image is recorded/rendered:
	struct {
		uint32_t width = 1280, height = 720; //pixels in the rendered image
		//path tracer parameters:
		uint32_t samples = 256; //how many samples to take per pixel
		uint32_t max_ray_depth = 8; //how deep rays can traverse
		//rasterizer parameters:
		uint32_t sample_pattern = 1; //supersampling pattern id
	} film;
};

bool operator!=(const Camera& a, const Camera& b);
