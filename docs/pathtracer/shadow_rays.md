---
layout: default
title: "(Task 4) Shadow Rays"
permalink: /pathtracer/shadow_rays
---

# (Task 4) Shadow Rays

In this task you will modify `Pathtracer::trace_ray` to implement accurate shadows.

Currently `Pathtracer::trace_ray` computes the following:

* It computes the intersection of ray `r` with the scene.
* It computes the amount of light arriving at the hit point `result.position` (the irradiance at the hit point) by integrating radiance from all scene light sources.
* It computes the radiance reflected from the hit point in the direction of -`r`. (The amount of reflected light is based on the BSDF of the surface at the hit point.)

Shadows occur when another scene object blocks light emitted from scene light sources towards the hit point. Fortunately, determining whether or not a ray of light from a light source to the hit point is occluded by another object is easy given a working ray tracer (which you have at this point!). **You simply want to know whether a ray originating from the hit point (`result.position`), and traveling towards the light source (direction to light) hits any scene geometry before reaching the light.** (Note that you need to consider light's distance from the hit point is given, more on this in the notes below.)

Your job is to implement the logic needed to compute whether hit point is in shadow with respect to the current light source sample. Below are a few notes:

* In the starter code, when we call `light.sample(result.position)`, it returns us a `Light_sample sample` at the hit point . (You might want to take a look at `rays/light.h` for the definition of `struct Light_sample` and `class light`.) A `Light_sample` contains fields `radiance`, `pdf`, `direction`, and `distance`. In particular, `sample.direction` is the direction from the hit point to the light source, and `sample.distance` is the distance from the hit point to the light source.

* A common ray tracing pitfall is for the "shadow ray" shot into the scene to accidentally hit the same objecr as `r` (the surface is erroneously determined to be occluded because the shadow ray is determined to hit the surface!). We recommend that you make sure the origin of the shadow ray is offset from the surface to avoid these erroneous "self-intersections". For example, consider setting the origin of the shadow ray to be `result.position + epsilon * sample.direction` instead of simply `result.position`. `EPS_F` is defined in for this purpose(see `lib/mathlib.h`).

* Another common pitfall is forgetting that it doesn't matter if the shadow ray hits any scene geometry after reaching the light. Note that the light's distance from the hit point is given by `sample.distance`. Also note that `Ray` has a member called `time_bound`...
* You will find it useful to debug your shadow code using the `DirectionalLight` since it produces hard shadows that are easy to reason about.
* You would want to comment out the line `Spectrum radiance_out = Spectrum(0.5f);` and initialize the `radiance_out` to a more reasonable value. Hint: is there supposed to have any amount of light before we even start considering each light sample?

At this point you should be able to render very striking images. For example, here is the head of Peter Schröder rendered with an area light from above.

![shadow_directional](new_results/shadow_peter.png)
