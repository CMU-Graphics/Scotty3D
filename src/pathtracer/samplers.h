#pragma once

#include "../lib/mathlib.h"
#include "../util/hdr_image.h"

struct RNG;

namespace Samplers {
//Samplers in Scotty3D allow picking random points in various geometric distributions.
// they provide sample() to generate samples
// and pdf() to report the probability density

//Point sampler: always samples exactly the same point
struct Point {
	Point(Vec3 point) : point(point) {
	}

	Vec3 sample(RNG &rng) const;
	float pdf(Vec3 at) const; //actually probability mass, not density, since this is a discrete distribution

	Vec3 point;
};

//Direction sampler: always samples exactly the same direction
using Direction = Point; //just use a Point sampler

//Rect sampler: uniformly samples from the [0,size.x]x[0,size.y] rectangle
struct Rect {
	Rect(Vec2 size = Vec2(1.0f)) : size(size) {
	}

	Vec2 sample(RNG &rng) const;
	float pdf(Vec2 at) const;

	Vec2 size;
};

// Circle sampler: uniformly sample a circle with a specified center and radius
struct Circle {
	Circle(Vec2 center = Vec2(0.f), float radius = 1.f) : center(center), radius(radius) {
	}

	Vec2 sample(RNG &rng) const;
	float pdf(Vec2 at) const;

	Vec2 center;
	float radius;
};

//Triangle sampler: uniformly sample the surface of a triangle
struct Triangle {
	Triangle(Vec3 v0, Vec3 v1, Vec3 v2) : v0(v0), v1(v1), v2(v2) {
	}

	Vec3 sample(RNG &rng) const;
	float pdf(Vec3 at) const;

	Vec3 v0, v1, v2;
};

//Hemisphere samplers sample the surface of a (y-up, radius-1) hemisphere:
namespace Hemisphere {

//Hemisphere::Uniform uniformly samples the surface:
struct Uniform {
	Uniform() = default;

	Vec3 sample(RNG &rng) const;
	float pdf(Vec3 at) const;
};

//Hemisphere::Cosine uses cosine-weighted sampling:
struct Cosine {
	Cosine() = default;

	Vec3 sample(RNG &rng) const;
	float pdf(Vec3 dir) const;
};
} // namespace Hemisphere

//Sphere samplers sample the surface of a unit sphere:
namespace Sphere {

//Sphere::Uniform uniformly samples the surface:
struct Uniform {
	Uniform() = default;

	Vec3 sample(RNG &rng) const;
	float pdf(Vec3 dir) const;

	Hemisphere::Uniform hemi;
};

//Sphere::Image importance-samples the surface, with importance given by a lat/lon image with the north pole at (0,1,0):
struct Image {
	Image() = default;
	Image(const HDR_Image& image);

	Vec3 sample(RNG &rng) const;
	float pdf(Vec3 dir) const;

	uint32_t w = 0, h = 0;
	std::vector<float> _pdf, _cdf;
	Rect jitter;
};

} // namespace Sphere
} // namespace Samplers
