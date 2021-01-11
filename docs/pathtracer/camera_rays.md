---
layout: default
title: "(Task 1) Generating Camera Rays"
permalink: /pathtracer/camera_rays
---

# (Task 1) Generating Camera Rays

"Camera rays" emanate from the camera and measure the amount of scene radiance that reaches a point on the camera's sensor plane. (Given a point on the virtual sensor plane, there is a corresponding camera ray that is traced into the scene.) 

Take a look at `Pathtracer::trace_pixel` in `student/pathtracer.cpp`. The job of this function is to compute the amount of energy arriving at this pixel of the image. Conveniently, we've given you a function `Pathtracer::trace_ray(r)` that provides a measurement of incoming scene radiance along the direction given by ray `r`. See `lib/ray.h` for the interface of ray.

When the number of samples per pixel is 1, you should sample incoming radiance at the center of each pixel by constructing a ray `r` that begins at this sensor location and travels through the camera's pinhole. Once you have computed this ray, then call `Pathtracer::trace_ray(r)` to get the energy deposited in the pixel. When supersampling is enabled, the expected behavior of the program is being addressed below.

Here are some [rough notes](https://drive.google.com/file/d/0B4d7cujZGEBqVnUtaEsxOUI4dTMtUUItOFR1alQ4bmVBbnU0/view) giving more detail on how to generate camera rays.

This tutorial from [Scratchapixel](https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays) also provides a detailed walkthrough of what you need to do. (Note that the coordinate convention that Scratchpixel adopted is different from the one we use, and you should stick to the coordinate system from the [rough notes](https://drive.google.com/file/d/0B4d7cujZGEBqVnUtaEsxOUI4dTMtUUItOFR1alQ4bmVBbnU0/view) all the time.)

**Step 1:** Given the width and height of the screen, and point in screen space, compute the corresponding coordinates of the point in normalized ([0-1]x[0-1]) screen space in `Pathtracer::trace_pixel`. Pass these coordinates to the camera via `Camera::generate_ray` in `camera.cpp`.

**Step 2:** Implement `Camera::generate_ray`. This function should return a ray **in world space** that reaches the given sensor sample point. We recommend that you compute this ray in camera space (where the camera pinhole is at the origin, the camera is looking down the -Z axis, and +Y is at the top of the screen.). In `util/camera.h`, the `Camera` class stores `vert_fov` and `aspect_ratio` indicating the vertical field of view of the camera (in degrees, not radians) as well as the aspect ratio. Note that the camera maintains camera-space-to-world space transform matrix `iview` that will come fairly handy. 

**Step 3:** Your implementation of `Pathtracer::trace_pixel` must support super-sampling. The starter code will call `Pathtracer::trace_pixel` one time for each sample (given by `Pathtracer::n_samples`), so your implementation of `Pathtracer::trace_pixel` should choose a new location within the pixel each time.

To choose a sample within the pixel, you should implement `Rect::Uniform::sample` (see `src/student/samplers.cpp`), such that it provides (random) uniformly distributed 2D points within the rectangular region specified by the origin and the member `Rect::Uniform::size`. You may then create a `Rect::Uniform` sampler with a one-by-one region and call `sample()` to obtain randomly chosen offsets within the pixel.

Once you have implemented `Pathtracer::trace_pixel`, `Rect::Uniform::sample` and `Camera::generate_ray`, you should have a working camera.

**Tip:**   Since it'll be hard to know if you camera rays are correct until you implement primitive intersection, we recommend debugging your camera rays by checking what your implementation of `Camera::generate_ray` does with rays at the center of the screen (0.5, 0.5) and at the corners of the image.

The code can log the results of raytracing for visualization and debugging. To do so, simply call function `Pathtracer::log_ray` in your `Pathtracer::trace_pixel`. Function `Pathtracer::log_ray` takes in 3 arguments: the ray thay you want to log, a float that specifies the time/distance to log that ray up to, as well as the color. You don't need to worry about the color as it is being set to white by default. You can only log only a portion of the generated rays for a cleaner visual effect, for example, do `if (RNG::coin_flip(0.0005f))log_ray(out, 10.0f);` to log 0.05% of generated camera rays.

You can visualize the result of the generated rays by checking the box for Logged rays under Visualize and then **start render**(Open Render Window -> Start Render).
After running the ray tracer, rays will be shown as lines in visualizer. Be sure to wait for rendering to complete so you see all rays while visualizing, a message is printed to standard output on completion. 

![logged_rays](new_results/log_rays.png)

**Extra credit ideas:**

* Modify the implementation of the camera to simulate a camera with a finite aperture (rather than a pinhole camera). This will allow your ray tracer to simulate the effect of defocus blur.
* Write your own sampler that generates samples with improved distribution. Some examples include:
  * Jittered Sampling
  * Multi-jittered sampling
  * N-Rooks (Latin Hypercube) sampling
  * Sobol sequence sampling
  * Halton sequence sampling
  * Hammersley sequence sampling
