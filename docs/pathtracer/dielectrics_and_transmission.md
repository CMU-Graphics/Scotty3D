---
layout: default
title: "Dielectrics and Transmission"
permalink: /pathtracer/dielectrics_and_transmission
---

# Dielectrics and Transmission

## Fresnel Equations for a Dielectric

The [Fresnel Equations](https://en.wikipedia.org/wiki/Fresnel_equations) (another [link](http://hyperphysics.phy-astr.gsu.edu/hbase/phyopt/freseq.html) here) describe the amount of reflection from a surface. The description below is an approximation for dielectric materials (materials that don't conduct electricity). In this assignment you're asked to implement a glass material, which is a dielectric.

In the description below, <img src="dielectric_eq1.png" width="15"> and ![dielectric_eq2](dielectric_eq2.png) refer to the index of refraction of the medium containing an incoming ray, and the zenith angle of the ray to the surface of a new medium. ![dielectric_eq3](dielectric_eq3.png) and ![dielectric_eq4](dielectric_eq4.png) refer to the index of refraction of the new medium and the angle to the surface normal of a transmitted ray.

The Fresnel equations state that reflection from a surface is a function of the surface's index of refraction, as well as the polarity of the incoming light. Since our renderer doesn't account for polarity, we'll apply a common approximation of averaging the reflectance of polarizes light in perpendicular and parallel polarized light:

![dielectric_eq5](dielectric_eq5.png)

The parallel and perpendicular terms are given by:

![dielectric_eq6](dielectric_eq6.png)

![dielectric_eq7](dielectric_eq7.png)

Therefore, for a dielectric material, the fraction of reflected light will be given by ![dielectric_eq8](dielectric_eq8.png), and the amount of transmitted light will be given by ![dielectric_eq9](dielectric_eq9.png).

Alternatively, you may compute![dielectric_eq8](dielectric_eq8.png)  using [Schlick's approximation](https://en.wikipedia.org/wiki/Schlick%27s_approximation).

## Distribution Function for Transmitted Light

We described the BRDF for perfect specular reflection in class, however we did not discuss the distribution function for transmitted light. Since refraction "spreads" or "condenses" a beam, unlike perfect reflection, the radiance along the ray changes due to a refraction event. In your assignment you should use Snell's Law to compute the direction of refraction rays, and use the following distribution function to compute the radiance of transmitted rays. We refer you guys to Pharr, Jakob, and and Humphries's book [Physically Based Rendering](http://www.pbr-book.org/) for a derivation based on Snell's Law and the relation ![dielectric_eq10](dielectric_eq10.png). (But you are more than welcome to attempt a derivation on your own!)
