# `A4T3` Linear Blend Skinning

Now that we have a skeleton set up, we need to link the skeleton to the mesh in order to get the mesh to follow the movements of the skeleton. We will implement linear blend skinning using `Skeleton::skin`, which uses weights stored on a mesh by `Skeleton::assign_bone_weights`, which in turn uses the helper `Skeleton::closest_point_on_line_segment`.

Linear blend skinning means that each vertex will be transformed to a weighted sum of its positions under various bone transformations (transformations from bind space to posed space):
$$v_i' = \sum_j w_{ij} P_j B_j^{-1} v_i$$

Where $P_j$ is the bone-to-pose transformation for bone $j$, $B_j$ is the bone-to-bind transformation for bone $j$, and $w_{ij}$ is the weight given to bone $j$ for vertex $i$. Note that if $\sum_j w_{ij} \ne 1$, the vertex will be scaled toward/away from the origin, so you should be cognizant of this when computing weights.

## Bone Weights

**Implement:**
- `Skeleton::closest_point_on_line_segment`, which does what it says in the function name.
- `Skeleton::assign_bone_weights`, which fills in the `bone_weights` vector for every vertex in a `Halfedge_Mesh`.

To run the equation above you need bone weights. These are stored in the `Halfedge_Mesh::Vertex::bone_weights` vector by the `Skeleton::assign_bone_weights` function when you press "calculate bone weights" in the UI.

There are many ways of computing bone weights. Your implementation should use the following method, which uses the `Bone::radius` parameter of bones to control their relative weights.

Let $d_{ij}$ be the distance from vertex $i$ to the closest point on bone $j$ (i.e. the segment from $0$ to $e_j$ in bone $j$'s local space).

<center><img src="T3/closest_on_line_segment.png" style="height:280px"></center>

Let $\hat{w}\_{ij}$ be $1$ when the point is on the bone and decrease to $0$ at the bone's radius, $r$:
$$\hat{w}\_{ij} \equiv \frac{\mathrm{max}(0, r-d_{ij})}{r}$$

Normalize such that the sum of the weights for any given vertex is one:
$$w_{ij} \equiv \frac{\hat{w}\_{ij}}{\sum_j \hat{w}\_{ij}}$$

(In case all weights are zero, do not store any weights for the vertex, and just transform it by the identity in `skin`.)

For efficiency, you should only store nonzero bone weights in `Vertex::bone_weights`.

## Skinning

**Implement:**
- `Skeleton::skin`, which applies a skining transform to a halfedge mesh and produces an indexed mesh.

Compute the resulting position of every vertex by doing a weighted average of the bind-to-posed transforms from each bone and applying it to the vertex.
Compute the normals of the adjacent corners by applying the [inverse transpose](https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html) of the transformation applied to the vertex.

Notice that `Skeleton::skin` outputs an indexed mesh. This is because the result of skinning is passed to drawing / rendering code that expects `Indexed_Mesh` structures, so there's no reason to stuff the results back into a `Halfedge_Mesh`. You may wish to read the `SplitEdges` case of `Indexed_Mesh::from_halfedge_mesh` for inspiration on how to structure this part of your `skin` function.
