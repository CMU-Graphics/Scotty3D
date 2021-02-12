---
layout: default
title: Ray Triangle Intersection
permalink: /pathtracer/ray_triangle_intersection
grand_parent: "A3: Pathtracer"
nav_order: 1
parent: (Task 2) Intersections
---

# Ray Triangle Intersection

We recommend that you implement the *Moller-Trumbore algorithm*, a fast algorithm
that takes advantage of a barycentric coordinates parameterization of the
intersection point, for ray-triangle intersection.

<center><img src="triangle_intersect_diagram.png" style="height:320px"></center>
<center><img src="triangle_intersect_eqns.png" style="height:400px"></center>

A few final notes and thoughts:

If the denominator _dot((e1 x d), e2)_ is zero, what does that mean about the relationship of the ray and the triangle? Can a triangle with this area be hit by a ray? Given _u_ and _v_, how do you know if the ray hits the triangle? Don't forget that the intersection point on the ray should be within the ray's `dist_bounds`.
