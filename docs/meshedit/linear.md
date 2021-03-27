---
layout: default
title: "Linear Subdivision"
permalink: /meshedit/global/linear/
parent: "Global Operations"
grand_parent: "A2: MeshEdit"
---

# Linear Subdivision

For an in-practice example, see the [User Guide](/Scotty3D/guide/model_mode).

Unlike most other global remeshing operations, linear (and Catmull-Clark) subdivision will proceed by completely replacing the original halfedge mesh with a new one. The high-level procedure is:

1.  Generate a list of vertex positions for the new mesh.
2.  Generate a list of polygons for the new mesh, as a list of indices into the new vertex list (a la "polygon soup").
3.  Using these two lists, rebuild the halfedge connectivity from scratch.

Given these lists, `Halfedge_Mesh::from_poly` will take care of allocating halfedges, setting up `next` and `twin` pointers, etc., based on the list of polygons generated in step 2---this routine is already implemented in the Scotty3D skeleton code.

Both linear and Catmull-Clark subdivision schemes will handle general _n_-gons (i.e., polygons with _n_ sides) rather than, say, quads only or triangles only. Each _n_-gon (including but not limited to quadrilaterals) will be split into _n_ quadrilaterals according to the following template:

<center><img src="subdivide_quad.png" style="height:220px"></center>

The high-level procedure is outlined in greater detail in `student/meshedit.cpp`.

### Vertex Positions

For global linear or Catmull-Clark subdivision, the strategy for assigning new vertex positions may at first appear a bit strange: in addition to updating positions at vertices, we will also calculate vertex positions associated with the _edges_ and _faces_ of the original mesh. Storing new vertex positions on edges and faces will make it extremely convenient to generate the polygons in our new mesh, since we can still use the halfedge data structure to decide which four positions get connected up to form a quadrilateral. In particular, each quad in the new mesh will consist of:

*   one new vertex associated with a face from the original mesh,
*   two new vertices associated with edges from the original mesh, and
*   one vertex from the original mesh.

For linear subdivision, the rules for computing new vertex positions are very simple:

*   New vertices at original faces are assigned the average coordinates of all corners of that face (i.e., the arithmetic mean).
*   New vertices at original edges are assigned the average coordinates of the two edge endpoints.
*   New vertices at original vertices are assigned the same coordinates as in the original mesh.

These values should be assigned to the members `Face::new_pos`, `Edge::new_pos`, and `Vertex::new_pos`, respectively. For instance, `f->new_pos = Vec3( x, y, z );` will assign the coordinates (x,y,z) to the new vertex associated with face `f`. The general strategy for assigning these new positions is to iterate over all vertices, then all edges, then all faces, assigning appropriate values to `new_pos`. **Note:** you _must_ copy the original vertex position `Vertex::pos` to the new vertex position `Vertex::new_pos`; these values will not be used automatically.

This step should be implemented in the method `Halfedge_Mesh::linear_subdivide_positions` in `student/meshedit.cpp`.

Steps 2 and 3 are already implemented by `Halfedge_Mesh::subdivide` in `geometry/halfedge.cpp`. For your understanding, an explanation of how these are implemented is provided below:

### Polygons

Recall that in linear and Catmull-Clark subdivision _all polygons are subdivided simultaneously_. In other words, if we focus on the whole mesh (rather than a single polygon), then we are globally

*   creating one new vertex for each edge,
*   creating one new vertex for each face, and
*   keeping all the vertices of the original mesh.

These vertices are then connected up to form quadrilaterals (_n_ quadrilaterals for each _n_-gon in the input mesh). Rather than directly modifying the halfedge connectivity, these new quads will be collected in a much simpler mesh data structure: a list of polygons. Note that with this subdivision scheme, _every_ polygon in the output mesh will be a quadrilateral, even if the input contains triangles, pentagons, etc.

In Scotty3D, a list of polygons can be declared as

    std::vector<std::vector<Index>> quads;

where `std::vector` is a [class from the C++ standard template library](http://en.cppreference.com/w/cpp/container/vector), representing a dynamically-sized array. An `Index` is just another name for a `size_t`, which is the standard C++ type for integers that specify an element of an array. Polygons can be created by allocating a list of appropriate size, then specifying the indices of each vertex in the polygon. For example:

    std::vector<Index> quad( 4 ); // allocate an array with four elements
    // Build a quad with vertices specified by integers (a,b,c,d), starting at zero.
    // These indices should correspond to the indices computing when assigning vertex
    // positions, as described above.
    quad[0] = a;
    quad[1] = b;
    quad[2] = c;
    quad[3] = d;

Once a quad has been created, it can be added to the list of quads by using the method `vector::push_back`, which appends an item to a vector:

    std::vector<std::vector<Index>> newPolygons;
    newPolygons.push_back( quad );

The full array of new polygons will then be passed to the method `Halfedge_Mesh::from_poly`, together with the new vertex positions.
