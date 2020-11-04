---
layout: default
title: "Ray Triangle Intersection"
permalink: /pathtracer/ray_triangle_intersection
---

# Ray Triangle Intersection

Given a triangle defined by points _p0_, _p1_ and _p2_ and a ray given by its origin _o_ and direction _d_, the barycentrics of the hit point as well as the _t_-value of the hit can be obtained by solving the system:

![triangle_eq1](triangle_eq1.png)

Where:

![triangle_eq2](triangle_eq2.png)

This system can be solved by Cramer's Rule, yielding:

![triangle_eq3](triangle_eq3.png)

In the above, |a b c| denotes the determinant of the 3x3 with column vectors _a_, _b_, _c_.
Note that since the determinant is given by:![triangle_eq4](triangle_eq4.png)
you can rewrite the above as:
![triangle_eq5](triangle_eq5.png)

Of which you should notice a few common subexpressions that, if exploited in an implementation, make computation of _t_, _u_, and _v_ substantially more efficient.

A few final notes and thoughts:

If the denominator _dot((e1 x d), e2)_ is zero, what does that mean about the relationship of the ray and the triangle? Can a triangle with this area be hit by a ray? Given _u_ and _v_, how do you know if the ray hits the triangle? Don't forget that the intersection point on the ray should be within the ray's `time_bound`.
