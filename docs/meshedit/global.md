---
layout: default
title: "Global Operations"
permalink: /meshedit/global/
parent: "A2: MeshEdit"
has_children: true
has_toc: false
nav_order: 2
---

# Global Mesh Operations

In addition to local operations on mesh connectivity, Scotty3D provides several global remeshing operations (as outlined in the [User Guide](/Scotty3D/guide/model_mode)). Two different mechanisms are used to implement global operations:

*   _Repeated application of local operations._ Some mesh operations are most easily expressed by applying local operations (edge flips, etc.) to a sequence of mesh elements until the target output is achieved. A good example is [mesh simplification](simplify), which is a greedy algorithm that collapses one edge at a time.
*   _Global replacement of the mesh._ Other mesh operations are better expressed by temporarily storing new mesh elements in a simpler mesh data structure (e.g., an indexed list of faces) and completely re-building the halfedge data structure from this data. A good example is [Catmull-Clark subdivision](catmull), where every polygon must be simultaneously split into quadrilaterals.

Note that in general there are no inter-dependencies among global remeshing operations (except that some of them require a triangle mesh as input, which can be achieved via the method `Halfedge_Mesh::triangulate`).

## Subdivision

In image processing, we often have a low resolution image that we want to display at a higher resolution. Since we only have a few samples of the original signal, we need to somehow interpolate or _upsample_ the image. One idea would be to simply cut each pixel into four, leaving the color values unchanged, but this leads to a blocky appearance. Instead we might try a more sophisticated scheme (like bilinear or trilinear interpolation) that yields a smoother appearance.

In geometry processing, one encounters the same situation: we may have a low-resolution polygon mesh that we wish to upsample for display, simulation, etc. Simply splitting each polygon into smaller pieces doesn't help, because it does nothing to alleviate blocky silhouettes or chunky features. Instead, we need an upsampling scheme that nicely interpolates or approximates the original data. Polygon meshes are quite a bit trickier than images, however, since our sample points are generally at _irregular_ locations, i.e., they are no longer found at regular intervals on a grid.

Three subdivision schemes are supported by Scotty3D: [Linear](linear), [Catmull-Clark](catmull), and [Loop](loop). The first two can be used on any polygon mesh without boundary, and should be implemented via the global replacement strategy described above. Loop subdivision can be implemented using repeated application of local operations. For further details, see the linked pages.

## Performance

All subdivision operations, as well as re-meshing and simplification, should complete almost instantaneously (no more than a second) on meshes of a few hundred polygons or fewer. If performance is worse than this, ensure that implementations are not repeatedly iterating over more elements than needed, or allocating/deallocating more memory than necessary. A useful debugging technique is to print out (or otherwise keep track of, e.g., via an integer counter or a profiler) the number of times basic methods like `Halfedge::next()` or `Halfedge_Mesh::new_vertex()` are called during a single execution of one of the methods; for most methods this number should be some reasonably small constant (no more than, say, 1000!) times the number of elements in the mesh.
