---
layout: default
title: Environment Light Importance Sampling
grand_parent: "A3: Pathtracer"
parent: (Task 7) Environment Lighting
permalink: /pathtracer/importance_sampling
---

# Environment Light Importance Sampling

A pixel with coordinate <img src="environment_eq1.png" style="height:18px"> subtends an area <img src="environment_eq2.png" style="height:18px"> on the unit sphere (where <img src="environment_eq3.png" style="height:16px"> and <img src="environment_eq4.png" style="height:18px"> the angles subtended by each pixel -- as determined by the resolution of the texture). Thus, the flux through a pixel is proportional to <img src="environment_eq5.png" style="height:14px">. (We only care about the relative flux through each pixel to create a distribution.)

**Summing the fluxes for all pixels, then normalizing the values so that they sum to one, yields a discrete probability distribution for picking a pixel based on flux through its corresponding solid angle on the sphere.**

The question is now how to sample from this 2D discrete probability distribution. We recommend the following process which reduces the problem to drawing samples from two 1D distributions, each time using the inversion method discussed in class:

* Given <img src="environment_eq6.png" style="height:18px"> the probability distribution for all pixels, compute the marginal probability distribution <img src="environment_eq7.png" style="height:32px"> for selecting a value from each row of pixels.

* Given for any pixel, compute the conditional probability <img src="environment_eq8.png" style="height:40px">.

Given the marginal distribution for <img src="environment_eq9.png" style="height:14px"> and the conditional distributions <img src="environment_eq10.png" style="height:18px"> for environment map rows, it is easy to select a pixel as follows:

1. Use the inversion method to first select a "row" of the environment map according to <img src="environment_eq11.png" style="height:18px">.
2. Given this row, use the inversion method to select a pixel in the row according to <img src="environment_eq12.png" style="height:18px">.
