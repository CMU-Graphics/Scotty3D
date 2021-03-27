---
layout: default
title: "Edge Flip Tutorial"
permalink: /meshedit/local/edge_flip
nav_order: 1
parent: Local Operations
grand_parent: "A2: MeshEdit"
---

# Edge Flip Tutorial

Here we provide a step-by-step guide to implementing a simplified version of the _EdgeFlip_ operation for a pair of triangles---the final version, however, must be implemented for general polygons (i.e., any _n_-gon). The basic strategy for implementing the other local operations is quite similar to the procedure outlined below.

**Note:** if you're not familiar with C++, you should definitely take a moment to learn about the [standard library class](http://en.cppreference.com/w/cpp/container/vector) `std::vector`, especially the method `push_back()`, which will make it easy to accumulate a list of pointers as you walk around a polygon, vertex, etc.

We now consider the case of a triangle-triangle edge flip.

### PHASE 0: Draw a Diagram

Suppose we have a pair of triangles (a,b,c) and (c,b,d). After flipping the edge (b,c), we should now have triangles (a,d,c) and (a,b,d). A good first step for implementing any local mesh operation is to draw a diagram that clearly labels all elements affected by the operation:

<center><img src="edge_flip_diagram.png"></center>

Here we have drawn a diagram of the region around the edge both before and after the edge operation (in this case, "flip"), labeling each type of element (halfedge, vertex, edge, and face) from zero to the number of elements. It is important to include every element affected by the operation, thinking very carefully about which elements will be affected. If elements are omitted during this phase, everything will break---even if the code written in the two phases is correct! In this example, for instance, we need to remember to include the halfedges "outside" the neighborhood, since their "twin" pointers will be affected.

### PHASE I: Collect elements

Once you've drawn your diagram, simply collect all the elements from the "before" picture. Give them the same names as in your diagram, so that you can debug your code by comparing with the picture.

    // HALFEDGES
    HalfedgeRef h0 = e0->halfedge();
    HalfedgeRef h1 = h0->next();
    HalfedgeRef h2 = h1->next();
    HalfedgeRef h3 = h0->twin();
    HalfedgeRef h4 = h3->next();
    HalfedgeRef h5 = h4->next();
    HalfedgeRef h6 = h1->twin();
    HalfedgeRef h7 = h2->twin();
    HalfedgeRef h8 = h4->twin();
    HalfedgeRef h9 = h5->twin();

    // VERTICES
    VertexRef v0 = h0->vertex();
    VertexRef v1 = h3->vertex();
    // ...you fill in the rest!...

    // EDGES
    EdgeRef e1 = h5->edge();
    EdgeRef e2 = h4->edge();
    // ...you fill in the rest!...

    // FACES
    FaceRef f0 = h0->face();
    // ...you fill in the rest!...

### PHASE II: Allocate new elements

If your edge operation requires new elements, now is the time to allocate them. For the edge flip, we don't need any new elements; but suppose that for some reason we needed a new vertex v4\. At this point we would allocate the new vertex via

    VertexRef v4 = mesh.new_vertex();

(The name used for this new vertex should correspond to the label you give it in your "after" picture.) Likewise, new edges, halfedges, and faces can be allocated via the methods `mesh.new_edge()`, `mesh.new_halfedge()`, and `mesh.new_face()`.

### PHASE III: Reassign Elements

Next, update the pointers for all the mesh elements that are affected by the edge operation. Be exhaustive! In other words, go ahead and specify every pointer for every element, even if it did not change. Once things are working correctly, you can always optimize by removing unnecessary assignments. But get it working correctly first! Correctness is more important than efficiency.

    // HALFEDGES
    h0->next() = h5;
    h0->twin() = h3;
    h0->vertex() = v3;
    h0->edge() = e0;
    h0->face() = f0;
    h1->next() = h0;
    h1->twin() = h6;
    h1->vertex() = v1;
    h1->edge() = e4;
    h1->face() = f0;
    // ...you fill in the rest!...

    // ...and don't forget about the "outside" elements!...
    h9->next() = h9->next(); // didn't change, but set it anyway!
    h9->twin() = h5;
    h9->vertex() = v1;
    h9->edge() = e1;
    h9->face() = h9->face(); // didn't change, but set it anyway!

    // VERTICES
    v0->halfedge() = h4;
    v1->halfedge() = h1;
    v2->halfedge() = h5;
    v3->halfedge() = h2;

    // EDGES
    e0->halfedge() = h0; //...you fill in the rest!...

    // FACES
    f0->halfedge() = h0; //...you fill in the rest!...

### PHASE IV: Delete unused elements

If your edge operation eliminates elements, now is the best time to deallocate them: at this point, you can be sure that they are no longer needed. For instance, since we do not need the vertex allocated in PHASE II, we could write

    mesh.erase(v4);

You should be careful that this mesh element is not referenced by any other element in the mesh. But if your "before" and "after" diagrams are correct, that should not be an issue!

### Design considerations

The basic algorithm outlined above will handle most edge flips, but you should also think carefully about possible corner-cases. You should also think about other design issues, like "how much should this operation cost?" For instance, for this simple triangle-triangle edge flip it might be reasonable to:

*   Ignore requests to flip boundary edges (i.e., just return immediately if either neighboring face is a boundary loop).
*   Ignore requests to perform any edge flip that would make the surface non-manifold or otherwise invalidate the mesh.
*   Not add or delete any elements. Since there are the same number of mesh elements before and after the flip, you should only need to reassign pointers.
*   Perform only a constant amount of work -- the cost of flipping a single edge should **not** be proportional to the size of the mesh!

Formally proving that your code is correct in all cases is challenging, but at least try to think about what could go wrong in degenerate cases (e.g., vertices of low degree, or very small meshes like a tetrahedron). The biggest challenge in properly implementing this type of local operation is making sure that all the pointers still point to the right place in the modified mesh, and will likely be the cause of most of your crashes! To help mitigate this, Scotty3D will automatically attempt to ``validate`` your mesh after each operation, and will warn you if it detects abnormalities. Note that it will still crash if you leave references to deleted mesh elements!
