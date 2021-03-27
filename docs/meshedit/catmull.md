---
layout: default
title: "Catmull-Clark Subdivision"
parent: Global Operations
grand_parent: "A2: MeshEdit"
permalink: /meshedit/global/catmull/
---

# Catmull-Clark Subdivision

For an in-practice example, see the [User Guide](/Scotty3D/guide/model_mode).

The only difference between Catmull-Clark and [linear](../linear) subdivision is the choice of positions for new vertices. Whereas linear subdivision simply takes a uniform average of the old vertex positions, Catmull-Clark uses a very carefully-designed _weighted_ average to ensure that the surface converges to a nice, round surface as the number of subdivision steps increases. The original scheme is described in the paper _"Recursively generated B-spline surfaces on arbitrary topological meshes"_ by (Pixar co-founder) Ed Catmull and James Clark. Since then, the scheme has been thoroughly discussed, extended, and analyzed; more modern descriptions of the algorithm may be easier to read, including those from the [Wikipedia](https://en.wikipedia.org/wiki/Catmull-Clark_subdivision_surface) and [this webpage](http://www.rorydriscoll.com/2008/08/01/catmull-clark-subdivision-the-basics/). In short, the new vertex positions can be calculated by:

1.  setting the new vertex position at each face f to the average of all its original vertices (exactly as in linear subdivision),
2.  setting the new vertex position at each edge e to the average of the new face positions (from step 1) and the original endpoint positions, and
3.  setting the new vertex position at each vertex v to the weighted sum


<center><img src="catmull_clark_positions.png" style="height:80px"></center>


where _n_ is the degree of vertex _v_ (i.e., the number of faces containing _v_), and

*   _Q_ is the average of all new face position for faces containing _v_,
*   _R_ is the average of all original edge midpoints for edges containing _v_, and
*   _S_ is the original vertex position for vertex _v_.

In other words, the new vertex positions are an "average of averages." (Note that you _will_ need to divide by _n_ _both_ when computing _Q_ and _R_, _and_ when computing the final, weighted value---this is not a typo!)

Apart from changing the way vertex positions are computed, there should be no difference in your implementation of linear and Catmull-Clark subdivision.

This step should be implemented in the method `HalfedgeMesh::catmullclark_subdivide_positions` in `student/meshedit.cpp`.

This subdivision rule **is not** required to support meshes with boundary, unless the implementer wishes to go above and beyond.
