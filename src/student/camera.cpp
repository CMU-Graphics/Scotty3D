
#include "../util/camera.h"
#include "../rays/samplers.h"
#include "debug.h"
#include <iostream>

float radians(float degrees) {
    float pi = 2*acos(0.0f);
    return degrees * pi / 180.0f;
}

Ray Camera::generate_ray(Vec2 screen_coord) const {

    // TODO (PathTracer): Task 1
    // compute position of the input sensor sample coordinate on the
    // canonical sensor plane one unit away from the pinhole.
    // Tip: compute the ray direction in view space and use
    // the camera transform to transform it back into world space.
    Samplers::Rect::Uniform samp;
    float p = 0.0f;
    float &pdf = p;
    auto height = 2*tan(radians(vert_fov)/2);
    auto width = 2*aspect_ratio*tan(radians(vert_fov)/2);
    Vec3 camdir (width *(screen_coord.x - 0.5f),height*(screen_coord.y - 0.5f), -1.0f);
    Vec2 rngs = samp.sample(pdf);
    Vec3 apertureadd (aperture * (rngs.x-0.5f), aperture * (rngs.y -0.5f), 0.0f);
    Vec3 origin = (aperture > 0) ? apertureadd : Vec3 (0,0,0);

    return Ray(iview * origin, iview.rotate(camdir*focal_dist - origin));
}
