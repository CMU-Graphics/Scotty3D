#pragma once

#include "introspect.h"
#include "../lib/mathlib.h"
#include "../platform/gl.h"
#include "../rasterizer/sample_pattern.h"

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
	uint32_t aperture_shape = 1;
	float aperture_size = 0.0f;
	float focal_dist = 1.f;

	//'film_' parameters specify how the image is recorded/rendered:
	struct {
		uint32_t width = 1280, height = 720; //pixels in the rendered image
		//path tracer parameters:
		uint32_t samples = 256; //how many samples to take per pixel
		uint32_t max_ray_depth = 8; //how deep rays can traverse
		//rasterizer parameters:
		uint32_t sample_pattern = 1; //supersampling pattern id
	} film;

	template< Intent I, typename F, typename C >
	static void introspect(F&& f, C&& c) {
		f("vertical_fov", c.vertical_fov);
		f("aspect_ratio", c.aspect_ratio);
		f("near_plane", c.near_plane);
		f("aperture_shape", c.aperture_shape);
		f("aperture_size", c.aperture_size);
		f("focal_dist", c.focal_dist);
		if constexpr (I != Intent::Animate) {
			f("film.width", c.film.width);
			f("film.height", c.film.height);
			f("film.samples", c.film.samples);
			f("film.max_ray_depth", c.film.max_ray_depth);
			//NOTE: might be null
			SamplePattern const *sample_pattern = SamplePattern::from_id(c.film.sample_pattern);
			f("film.sample_pattern", sample_pattern);
			if constexpr (I == Intent::Write) {
				if (sample_pattern != nullptr) {
					c.film.sample_pattern = sample_pattern->id;
				} else {
					Camera def;
					c.film.sample_pattern = def.film.sample_pattern;
					warn("Camera with no sample pattern, defaulting to %u", c.film.sample_pattern);
				}
			}
		}
	}
	static inline const char *TYPE = "Camera";
};

bool operator!=(const Camera& a, const Camera& b);
