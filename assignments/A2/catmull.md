# `A2G3` Catmull-Clark Subdivision

The only difference between Catmull-Clark and [linear](linear.md) subdivision is the choice of positions for new vertices. Whereas linear subdivision simply takes a uniform average of the old vertex positions, Catmull-Clark uses a very carefully-designed _weighted_ average to ensure that the surface converges to a nice, round surface as the number of subdivision steps increases. The original scheme is described in the paper _"Recursively generated B-spline surfaces on arbitrary topological meshes"_ by (Pixar co-founder) Ed Catmull and James Clark. Since then, the scheme has been thoroughly discussed, extended, and analyzed; more modern descriptions of the algorithm may be easier to read, including those from the [Wikipedia](https://en.wikipedia.org/wiki/Catmull-Clark_subdivision_surface) and [this webpage](http://www.rorydriscoll.com/2008/08/01/catmull-clark-subdivision-the-basics/). In short, the new vertex positions can be calculated by:

1.  setting the new vertex position at each face $f$ to the average of all its original vertices (exactly as in linear subdivision),
2.  setting the new vertex position at each edge $e$ to the average of the new adjacent face positions (from step 1) **and** the original edge endpoint positions, and
3.  setting the new vertex position at each vertex $v$ to the weighted sum


$$\frac{Q+2R+(n-3)S}{n}$$


where $n$ is the degree of vertex $v$ (i.e., the number of faces containing $v$), and

*   $Q$ is the average of all new face position for faces containing $v$,
*   $R$ is the average of all original edge midpoints for edges containing $v$, and
*   $S$ is the original vertex position for vertex $v$.

In other words, the new vertex positions are an "average of averages." (Note that you _will_ need to divide by $n$ _both_ when computing $Q$ and $R$, _and_ when computing the final, weighted value---this is not a typo!)

