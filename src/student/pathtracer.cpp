
#include "../rays/pathtracer.h"
#include "../rays/samplers.h"
#include "../util/rand.h"
#include "debug.h"

namespace PT {

Spectrum Pathtracer::trace_pixel(size_t x, size_t y) {

    Vec2 xy((float)x, (float)y);
    Vec2 wh((float)out_w, (float)out_h);

    // TODO (PathTracer): Task 1

    // Generate a sample within the pixel with coordinates xy and return the
    // incoming light using trace_ray.

    // Tip: Samplers::Rect::Uniform
    // Tip: you may want to use log_ray for debugging

    // This currently generates a ray at the bottom left of the pixel every time.

    Ray out = camera.generate_ray(xy / wh);
    out.depth = max_depth;
    return trace_ray(out);
}

Spectrum Pathtracer::trace_ray(const Ray &ray) {

    // Trace ray into scene. If nothing is hit, sample the environment
    Trace hit = scene.hit(ray);
    if (!hit.hit) {
        if (env_light.has_value()) {
            return env_light.value().sample_direction(ray.dir);
        }
        return {};
    }

    // Set up a coordinate frame at the hit point, where the surface normal becomes {0, 1, 0}
    // This gives us out_dir and later in_dir in object space, where computations involving the
    // normal become much easier. For example, cos(theta) = dot(N,dir) = dir.y!
    Mat4 object_to_world = Mat4::rotate_to(hit.normal);
    Mat4 world_to_object = object_to_world.T();
    Vec3 out_dir = world_to_object.rotate(ray.point - hit.position).unit();
    const BSDF &bsdf = materials[hit.material];

    // Now we can compute the rendering equation at this point.
    // We split it into two stages: sampling lighting (i.e. directly connecting
    // the current path to each light in the scene), then sampling the BSDF
    // to create a new path segment.

    // TODO (PathTracer): Task 5
    // Instead of initializing this value to a constant color, use the direct,
    // indirect lighting components calculated in the code below. The starter
    // code sets radiance_out to (0.5,0.5,0.5) so that you can test your geometry
    // queries before you implement path tracing.
    Spectrum radiance_out =
        debug_data.normal_colors ? Spectrum::direction(hit.normal) : Spectrum(0.5f);
    {
        auto sample_light = [&](const auto &light) {
            // If the light is discrete (e.g. a point light), then we only need
            // one sample, as all samples will be equivalent
            int samples = light.is_discrete() ? 1 : (int)n_area_samples;
            for (int i = 0; i < samples; i++) {

                Light_Sample sample = light.sample(hit.position);
                Vec3 in_dir = world_to_object.rotate(sample.direction);

                // If the light is below the horizon, ignore it
                float cos_theta = in_dir.y;
                if (cos_theta <= 0.0f)
                    continue;

                // If the BSDF has 0 throughput in this direction, ignore it
                // This is another oppritunity to do Russian roulette on low-throughput rays,
                // which would allow us to skip the shadow ray cast, increasing efficiency.
                Spectrum absorbsion = bsdf.evaluate(out_dir, in_dir);
                if (absorbsion.luma() == 0.0f)
                    continue;

                // TODO (PathTracer): Task 4
                // Construct a shadow ray and compute whether the intersected surface is
                // in shadow. Only accumulate light if not in shadow.

                // Tip: when making your ray, you will want to slightly offset it from the
                // surface it starts on, lest it intersect at time=0. Similarly, you may want
                // to limit the ray slightly before it would hit the light itself.

                // Note: that along with the typical cos_theta, pdf factors, we divide by samples.
                // This is because we're  doing another monte-carlo estimate of the lighting from
                // area lights.
                radiance_out += (cos_theta / (samples * sample.pdf)) * sample.radiance * absorbsion;
            }
        };

        // If the BSDF is discrete (i.e. uses dirac deltas/if statements), then we are never
        // going to hit the exact right direction by sampling lights, so ignore them.
        if (!bsdf.is_discrete()) {
            for (const auto &light : lights)
                sample_light(light);
            if (env_light.has_value())
                sample_light(env_light.value());
        }
    }

    // TODO (PathTracer): Task 5
    // Compute an indirect lighting estimate using pathtracing with Monte Carlo.

    // (1) Ray objects have a depth field; you should use this to avoid
    // traveling down one path forever.

    // (2) randomly select a new ray direction (it may be reflection or transmittence
    // ray depending on surface type) using bsdf.sample()

    // (3) potentially terminate path (using Russian roulette). You can make this
    // a function of the bsdf attenuation or track overall ray throughput

    // (4) create new scene-space ray and cast it to get incoming light

    // (5) add contribution due to incoming light with proper weighting
    return radiance_out;
}

} // namespace PT
