---
layout: default
title: "Bevelling"
permalink: /meshedit/local/bevel/
parent: "Local Operations"
grand_parent: "A2: MeshEdit"
---

# Beveling

Here we provide some additional detail about the bevel operations and their implementation in Scotty3D. Each bevel operation has two components:

1.  a method that modifies the _connectivity_ of the mesh, creating new beveled elements, and
2.  a method the updates the _geometry_ of the mesh, insetting and offseting the new vertices according to user input.

The methods that update the connectivity are `HalfedgeMesh::bevel_vertex`, `halfedgeMesh::bevel_edge`, and `HalfedgeMesh::bevel_face`. The methods that update geometry are `HalfedgeMesh::bevel_vertex_positions`, `HalfedgeMesh::bevel_edge_positions`, and `HalfedgeMesh::bevel_face_positions`.

The methods for updating connectivity can be implemented following the general strategy outlined in [edge flip tutorial](edge_flip). **Note that the methods that update geometry will be called repeatedly for the same bevel, in order to adjust positions according to user mouse input. See the gif in the [User Guide](/Scotty3D/guide/model_mode).**

To update the _geometry_ of a beveled element, you are provided with the following data:

*   `start_positions` - These are the original vertex positions of the beveled mesh element, without any insetting or offsetting.
*   `face` - This is a reference to the face currently being beveled. This was returned by one of the connectivity functions.
*   `tangent_offset` - The amount by which the new face should be inset (i.e., "shrunk" or "expanded")
*   `normal_offset` - (faces only) The amount by which the new face should be offset in the normal direction.

Also note that we provide code to gather the halfedges contained in the beveled face, creating the array `new_halfedges`. You should only have to update the position (`Vertex::pos`) of the vertices associated with this list of halfedges. The basic recipe for updating these positions is:

*   Iterate over the list of halfedges (`new_halfedges`)
*   Grab the vertex coordinates that are needed to compute the new, updated vertex coordinates (this could be a mix of values from `start_positions`, or the members `Vertex::pos`)
*   Compute the updated vertex positions using the current values of `tangent_offset` (and possibly `normal_offset`)
*   Store the new vertex positions in `Vertex::pos` _for the vertices of the new, beveled polygon only_ (i.e., the vertices associated with each of `new_halfedges`).

The reason for storing `new_halfedges` and `start_positions` in an array is that it makes it easy to access positions "to the left" and "to the right" of a given vertex. For instance, suppose we want to figure out the offset from the corner of a polygon. We might want to compute some geometric quantity involving the three vertex positions `start_positions[i-1]`, `start_positions[i]`, and `start_positions[i+1]` (as well as `inset`), then set the new vertex position `new_halfedges[i]->vertex()->pos` to this new value:

<center><img src="bevel_diagram.png"></center>

A useful trick here is _modular arithmetic_: since we really have a "loop" of vertices, we want to make sure that indexing the next element (+1) and the previous element (-1) properly "wraps around." This can be achieved via code like

    // Get the number of vertices in the new polygon
    int N = (int)hs.size();

    // Assuming we're looking at vertex i, compute the indices
    // of the next and previous elements in the list using
    // modular arithmetic---note that to get the previous index,
    // we can't just subtract 1 because the mod operation in C++
    // doesn't behave quite how you might expect for negative
    // values!
    int a = (i+N-1) % N;
    int b = i;
    int c = (i+1) % N;

    // Get the actual 3D vertex coordinates at these vertices
    Vec3 pa = start_positions[a];
    Vec3 pb = start_positions[b];
    Vec3 pc = start_positions[c];

From here, you will need to compute new coordinates for vertex `i`, which can be accessed from `new_halfedges[i]->vertex()->pos`. As a "dummy" example (i.e., this is NOT what you should actually do!!) this code will set the position of the new vertex to the average of the vertices above:

    new_halfedges[i].vertex()->pos = ( pa + pb + pc ) / 3.; // replace with something that actually makes sense!

The only question remaining is: where _should_ you put the beveled vertex? **We will leave this decision up to you.** This question is one where you will have to think a little bit about what a good design would be. Questions to ask yourself:

*   How do I compute a point that is inset from the original geometry?
*   For faces, how do I shift the geometry in the normal direction? (You may wish to use the method `Face::normal()` here.)
*   What should I do as the offset geometry starts to look degenerate, e.g., shrinks to a point, or goes outside some reasonable bounds?
*   What should I do when the geometry is nonplanar?
*   Etc.

The best way to get a feel for whether you have a good design is _to try it out!_ Can you successfully and easily use this tool to edit your mesh? Or is it a total pain, producing bizarre results? You be the judge!
