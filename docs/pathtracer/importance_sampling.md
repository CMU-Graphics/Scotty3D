---
layout: default
title: "Environment Light Importance Sampling"
permalink: /pathtracer/importance_sampling
---

# Environment Light Importance Sampling

A pixel with coordinate ![environment_eq1](environment_eq1.png) subtends an area ![environment_eq2](environment_eq2.png) on the unit sphere (where ![environment_eq3](environment_eq3.png) and ![environment_eq4](environment_eq4.png) the angles subtended by each pixel -- as determined by the resolution of the texture). Thus, the flux through a pixel is proportional to ![environment_eq5](environment_eq5.png). (We only care about the relative flux through each pixel to create a distribution.)

**Summing the fluxes for all pixels, then normalizing the values so that they sum to one, yields a discrete probability distribution for picking a pixel based on flux through its corresponding solid angle on the sphere.**

The question is now how to sample from this 2D discrete probability distribution. We recommend the following process which reduces the problem to drawing samples from two 1D distributions, each time using the inversion method discussed in class:

* Given ![environment_eq6](environment_eq6.png) the probability distribution for all pixels, compute the marginal probability distribution ![environment_eq7](environment_eq7.png) for selecting a value from each row of pixels.

* Given for any pixel, compute the conditional probability ![environment_eq8](environment_eq8.png).

Given the marginal distribution for ![environment_eq9](environment_eq9.png) and the conditional distributions ![environment_eq10](environment_eq10.png) for environment map rows, it is easy to select a pixel as follows:

1. Use the inversion method to first select a "row" of the environment map according to ![environment_eq11](environment_eq11.png).
2. Given this row, use the inversion method to select a pixel in the row according to ![environment_eq12](environment_eq12.png).