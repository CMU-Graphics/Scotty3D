---
layout: default
title: (Task 2) Intersections
permalink: /pathtracer/intersecting_objects
parent: "A3: Pathtracer"
has_children: true
has_toc: false
---

# (Task 2) Intersecting Objects

Now that your ray tracer generates camera rays, we need to be able to answer the core query in ray tracing: "does this ray hit this object?" Here, you will start by implementing ray-object intersection routines against the two types of objects in the starter code: **triangles** and **spheres**.

First, take a look at `rays/object.h` for the interface of the `Object` class. An `Object` can be **either** a `Tri_Mesh`, a `Shape`, a BVH(which you will implement in Task 3), or a list of `Objects`. Right now, we are only dealing with `Tri_Mesh`'s case and `Shape`'s case, and their interfaces are in `rays/tri_mesh.h`  and `rays/shapes.h`, respectively. `Tri_Mesh` contains a BVH of `Triangle`, and in this task you will be working with the `Triangle` class. For `Shape`, you are going to work with `Sphere`s, which is the major type of `Shape` in Scotty 3D.

Now, you need to implement the `hit` routine for both `Triangle` and `Sphere`. `hit` takes in a ray, and returns a `Trace` structure, which contains information on whether the ray hits the object and if hits, the information describing the surface at the point of the hit. See `rays/trace.h` for the definition of `Trace`.

In order to correctly implement `hit` you need to understand some of the fields in the Ray structure defined in `lib/ray.h`.

* `point`: the 3D point of origin of the ray
* `dir`: the 3D direction of the ray (always normalized)
* `dist_bounds`: the minimum and maximum distance along the ray. Primitive intersections that lie outside the [`ray.dist_bounds.x`, `ray.dist_bounds.y`] range should be disregarded.
* `depth`: the recursive depth of the ray (Used in task 5).
* `throughput`: the fraction of incoming light along this ray that will contribute to the final image (Used in task 5).

One important detail of the ray structure is that `dist_bounds` is a mutable field. This means that it can be modified even in `const` rays, for example within `Triangle::hit`. When finding the first intersection of a ray and the scene, you will want to update the ray's `dist_bounds` value after finding each hit with scene geometry. By bounding the ray as tightly as possible, your ray tracer will be able to avoid unnecessary tests with scene geometry that is known to not be able to result in a closest hit, resulting in higher performance.

---

## Step 1: `Triangle::hit`

The first intersect routine that the `hit` routines for the triangle mesh in `student/tri_mesh.cpp`.

While faster implementations are possible, we recommend you implement ray-triangle intersection using the method described in the [lecture slides](http://15462.courses.cs.cmu.edu/fall2017/lecture/acceleratingqueries). Further details of implementing this method efficiently are given in [these notes](ray_triangle_intersection.md).

There are two important details you should be aware of about intersection:

* When finding the first-hit intersection with a triangle, you need to fill in the `Trace` structure with details of the hit. The structure should be initialized with:

    * `hit`: a boolean representing if there is a hit or not.
    * `distance`: the distance from the origin of the ray to the hit point.
    * `position`: the position of the hit point. This may also be computed from `distance` and the query ray.
    * `normal`: the normal of the surface at the hit point. If the intersection is with a triangle, the normal should be interpolated from per-vertex normals according to the barycentric coordinates of the hit point.
    * `origin`: the origin of the query ray (ignore).
    * `material`: the material ID of the hit object (ignore).

Once you've successfully implemented triangle intersection, you will be able to render many of the scenes in the media directory. However, your ray tracer will be very slow!

**Tip:** While you are working with `student/tri_mesh.cpp`, you can choose to implement `Triangle::bbox` as well (pretty straightforward to do), which is needed for task 3.

## Step 2: `Sphere::hit`

You also need to implement the `hit` routines for the `Sphere` class in `student/shapes.cpp`. Remember that your intersection tests should respect the ray's `dist_bound`, and that normals should be out-facing.

**Tip 1:** take care **NOT** to use the `Vec3::normalize()` method when computing your
normal vector. You should instead use `Vec3::unit()`, since `Vec3::normalize()`
will actually change the `Vec3` calling object rather than returning a
normalized version.


**Tip 2:** A common mistake is to forget to check the case where the first
interesection time t1 is out of bounds but the second interesection time t2 is
(in which case you should return t2).

---

[Visualization of normals](visualization_of_normals.md) might be very helpful with debugging.
