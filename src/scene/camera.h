#pragma once

#include "../lib/mathlib.h"
#include "../platform/gl.h"

struct RNG;

class Camera {
public:
	std::pair<Ray, float> sample_ray(RNG &rng, uint32_t px, uint32_t py);
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
