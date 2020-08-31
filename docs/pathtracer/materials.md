---
layout: default
title: "(Task 6) Materials"
permalink: /pathtracer/materials
---

# (Task 6) Materials

Now that you have implemented the ability to sample more complex light paths, it's finally time to add support for more types of materials (other than the fully Lambertian material that you have implemented in Task 5). In this task you will add support for two types of materials: a perfect mirror and glass (a material featuring both specular reflection and transmittance) in `student/bsdf.cpp`.

To get started take a look at the BSDF interface in `rays/bsdf.h`. There are a number of key methods you should understand in `BSDF class`:

* `Spectrum evaluate(Vec3 out_dir, Vec3 in_dir)`: evaluates the distribution function for a given pair of directions.
* `BSDF_Sample sample(Vec3 out_dir)`: given the `out_dir`, generates a random sample of the in-direction (which may be a reflection direction or a refracted transmitted light direction). It returns a `BSDF_Sample`, which contains the in-direction(`direction`), its probability (`pdf`), as well as the `attenuation` for this pair of directions. (You do not need to worry about the `emissive` for the materials that we are asking you to implement, since those materials do not emit light.)

There are also two helper functions in the BSDF class in `student/bsdf.cpp` that you will need to implement:

* `Vec3 reflect(Vec3 dir)` returns a direction that is the **perfect specular reflection** direction corresponding to `dir` (reflection of `dir` about the normal, which in the surface coordinate space is [0,1,0]). More detail about specular reflection is [here](http://15462.courses.cs.cmu.edu/fall2015/lecture/reflection/slide_028).

* `Vec3 refract(Vec3 out_dir, float index_of_refraction, bool& was_internal)` returns the ray that results from refracting the ray in `out_dir` about the surface according to [Snell's Law](http://15462.courses.cs.cmu.edu/fall2015/lecture/reflection/slide_032). The surface's index of refraction is given by the argument `index_of_refraction`. Your implementation should assume that if the ray in `out_dir` **is entering the surface** (that is, if `cos(out_dir, N=[0,1,0]) > 0`) then the ray is currently in vacuum (index of refraction = 1.0). If `cos(out_dir, N=[0,1,0]) < 0` then your code should assume the ray is leaving the surface and entering vacuum. **In the case of total internal reflection, you should set `*was_internal` to `true`.**

* Note that in `reflect` and `refract`, both the `out_dir` and the returned in-direction are pointing away from the intersection point of the ray and the surface, as illustrated in this picture below.
![rays_dir](rays_dir.png)

## Step 1

Implement the class `BSDF_Mirror` which represents a material with perfect specular reflection (a perfect mirror). You should Implement `BSDF_Mirror::sample`, `BSDF_Mirror::evaluate`, and `reflect`. **(Hint: what should the pdf sampled by  `BSDF_Mirror::sample` be? What should the reflectance function `BSDF_Mirror::evalute` be?)**

## Step 2

Implement the class `BSDF_Glass` which is a glass-like material that both reflects light and transmit light. As discussed in class the fraction of light that is reflected and transmitted through glass is given by the dielectric Fresnel equations, which are [documented in detail here](dielectrics_and_transmission.md). Specifically your implementation should:

* Implement `refract` to add support for refracted ray paths.
* Implement `BSDF_refract::sample` as well as `BSDF_Glass::sample`. Your implementation should use the Fresnel equations to compute the fraction of reflected light and the fraction of transmitted light. The returned ray sample should be either a reflection ray or a refracted ray, with the probability of which type of ray to use for the current path proportional to the Fresnel reflectance. (e.g., If the Fresnel reflectance is 0.9, then you should generate a reflection ray 90% of the time. What should the pdf be in this case?) Note that you can also use [Schlick's approximation](https://en.wikipedia.org/wiki/Schlick's_approximation) instead.
* You should read the [provided notes](dielectrics_and_transmission.md) on the Fresnel equations as well as on how to compute a transmittance BSDF.
When you are done, you will be able to render images like these:

When you are done, you will be able to render images like these:

![cornell_classic](cornell_classic.png)

