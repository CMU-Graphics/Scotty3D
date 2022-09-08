
#include "../geometry/halfedge.h"

#include <set>
#include <unordered_map>
#include <vector>

/******************************************************************
*********************** Local Operations **************************
******************************************************************/

/* Note on local operation return types:

    The local operations all return a std::optional<T> type. This is used so that your
    implementation can signify that it does not want to perform the operation for
    whatever reason (e.g. you don't want to allow the user to erase the last vertex).

    An optional can have two values: std::nullopt, or a value of the type it is
    parameterized on. In this way, it's similar to a pointer, but has two advantages:
    the value it holds need not be allocated elsewhere, and it provides an API that
    forces the user to check if it is null before using the value.

    In your implementation, if you have successfully performed the operation, you can
    simply return the required reference:

            ... collapse the edge ...
            return collapsed_vertex_ref;

    And if you wish to deny the operation, you can return the null optional:

            return std::nullopt;

    Note that the stubs below all reject their duties by returning the null optional.
*/

/*
    This method splits the given edge in half, but does not split the
    adjacent faces. Returns an iterator to the new vertex which splits
    the original edge.

	Example function for how to go about implementing local operations
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::bisect_edge(EdgeRef e) {

	// Phase 1: collect all elements
	HalfedgeRef h = (e->halfedge->is_boundary()) ? e->halfedge->twin : e->halfedge;
	HalfedgeRef ht = h->twin;
	HalfedgeRef preh = h;
	HalfedgeRef nexht = ht->next;
	do {
		preh = preh->next;
	} while (preh->next != h);
	Vec3 vpos = (h->vertex->position + ht->vertex->position) / 2;

	// Phase 2: Allocate new elements
	VertexRef c = new_vertex();
	c->position = vpos;
	HalfedgeRef hn = new_halfedge();
	HalfedgeRef hnt = new_halfedge();
	EdgeRef e0 = new_edge();

	// The following elements aren't necessary for the bisect_edge, but they are here to demonstrate phase 4
    FaceRef f_not_used = new_face();
    HalfedgeRef h_not_used = new_halfedge();

	// Phase 3: Reassign elements
	e0->halfedge = hn;
	hn->twin = hnt;
	hnt->twin = hn;
	hn->edge = e0;
	hnt->edge = e0;
	hn->vertex = h->vertex;
	hnt->vertex = c;
	hn->face = h->face;
	hnt->face = ht->face;
	preh->next = hn;
	hn->next = h;
	h->vertex = c;
	ht->next = hnt;
	hnt->next = nexht;
	c->halfedge = h;
	hn->vertex->halfedge = hn;
	// is_new parameter is used for global operations
	c->is_new = true;

	// Phase 4: Delete unused elements
    erase(f_not_used);
    erase(h_not_used);

	// Phase 5: Return the correct iterator
	return c;
}

/*
    Merge all faces incident on a given vertex, returning a
    pointer to the merged face.
 */
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::erase_vertex(Halfedge_Mesh::VertexRef v) {

	// A2 Local(OPTIONAL): erase_vertex

	// Implement this for extra credit!
	(void)v;
    return std::nullopt;
}

/*
    Merge the two faces on either side of an edge, returning a
    pointer to the merged face.
 */
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::erase_edge(Halfedge_Mesh::EdgeRef e) {

	// A2 Local(OPTIONAL): erase_edge
	
	// Implement this for extra credit!
	(void)e;
    return std::nullopt;
}

/*
    Collapse an edge, returning a pointer to the collapsed vertex
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::collapse_edge(Halfedge_Mesh::EdgeRef e) {

	// A2 Local(REQUIRED): collapse_edge
	
	(void)e;
    return std::nullopt;
}

/*
    Collapse a face, returning a pointer to the collapsed vertex
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::collapse_face(Halfedge_Mesh::FaceRef f) {

	// A2 Local(OPTIONAL): collapse_face
	
	// Implement this for extra credit!
	(void)f;
    return std::nullopt;
}

/*
    Flip an edge, returning a pointer to the flipped edge
*/
std::optional<Halfedge_Mesh::EdgeRef> Halfedge_Mesh::flip_edge(Halfedge_Mesh::EdgeRef e) {

	// A2 Local(REQUIRED): flip_edge
	
	(void)e;
    return std::nullopt;
}

/*
    Split an edge, returning a pointer to the inserted midpoint vertex; the
    halfedge of this vertex should refer to one of the edges in the original
    mesh
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::split_edge(Halfedge_Mesh::EdgeRef e) {

	// A2 Local(REQUIRED): split_edge
	
	(void)e;
    return std::nullopt;
}

/*
    Insets a vertex into the given face, returning a pointer to the new center vertex
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::inset_vertex(FaceRef f) {

	// A2 Local(OPTIONAL): inset_vertex
	
	// Implement this for extra credit!
	(void)f;
    return std::nullopt;
}

/* Note on the beveling process:

    Each of the bevel_vertex, bevel_edge, and bevel_face functions do not represent
    a full bevel operation. Instead, they should update the _connectivity_ of
    the mesh, _not_ the positions of newly created vertices. In fact, you should set
    the positions of new vertices to be exactly the same as wherever they "started from."

    When you click on a mesh element while in bevel mode, one of those three functions
    is called. But, because you may then adjust the distance/offset of the newly
    beveled face, we need another method of updating the positions of the new vertices.

    This is where bevel_vertex_positions, bevel_edge_positions, and
    bevel_face_positions come in: these functions are called repeatedly as you
    move your mouse, the position of which determines the normal and tangent offset
    parameters. These functions are also passed an array of the original vertex
    positions: for bevel_vertex, it has one element, the original vertex position,
    for bevel_edge, two for the two vertices, and for bevel_face, it has the original
    position of each vertex in order starting from face->halfedge. You should use these 
    positions, as well as the normal and tangent offset fields to assign positions to 
    the new vertices.

    Finally, note that the normal and tangent offsets are not relative values - you
    should compute a particular new position from them, not a delta to apply.
*/

/*
    Creates a face in place of the vertex, returning a pointer to the new face
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_vertex(Halfedge_Mesh::VertexRef v) {

	// A2 Local(OPTIONAL): bevel_vertex
	
	// Implement this for extra credit!
	// Reminder: This function does not update the vertex positions.
	(void)v;
    return std::nullopt;
}

/*
    Creates a face in place of the edge, returning a pointer to the new face
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_edge(Halfedge_Mesh::EdgeRef e) {

	// A2 Local(OPTIONAL): bevel_edge
	
	// Implement this for extra credit!
	// Reminder: This function does not update the vertex positions.
	(void)e;
    return std::nullopt;
}

/*
    Insets a face into the given face, returning a pointer to the new center face
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_face(Halfedge_Mesh::FaceRef f) {

	// A2 Local(REQUIRED): bevel_face
	
	// Reminder: This function does not update the vertex positions.
	(void)f;
    return std::nullopt;
}

/*
    Compute new vertex positions for the vertices of the beveled vertex.

	These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

	The basic strategy here is to loop over the list of outgoing halfedges,
    and use the original vertex position and its associated outgoing edge
    to compute a new vertex position along the outgoing edge.
*/
void Halfedge_Mesh::bevel_vertex_positions(const std::vector<Vec3>& start_positions,
                                           Halfedge_Mesh::FaceRef face, float tangent_offset) {

	// A2 Local(OPTIONAL): bevel_vertex_positions
	
	// Implement this for extra credit!
	std::vector<HalfedgeRef> new_halfedges;
	auto h = face->halfedge;
	do {
		new_halfedges.push_back(h);
		h = h->next;
	} while (h != face->halfedge);

	(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;
}

/*
    Compute new vertex positions for the vertices of the beveled edge.

	These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the preceding and next vertex position from the original mesh
    (in the orig array) to compute an offset vertex position.

    Note that there is a 1-to-1 correspondence between halfedges in
    newHalfedges and vertex positions in start_positions. So, you can write 
    loops of the form:

    for(size_t i = 0; i < new_halfedges.size(); i++)
    {
            Vector3D pi = start_positions[i]; // get the original vertex
            position corresponding to vertex i
    }
*/
void Halfedge_Mesh::bevel_edge_positions(const std::vector<Vec3>& start_positions,
                                         Halfedge_Mesh::FaceRef face, float tangent_offset) {

	// A2 Local(OPTIONAL): bevel_edge_positions
	
	// Implement this for extra credit!
	std::vector<HalfedgeRef> new_halfedges;
	auto h = face->halfedge;
	do {
		new_halfedges.push_back(h);
		h = h->next;
	} while (h != face->halfedge);

	(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;
}

/*
    Compute new vertex positions for the vertices of the beveled face.
    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 0, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the preceding and next vertex position from the original mesh
    (in the start_positions array) to compute an offset vertex
    position.

    Note that there is a 1-to-1 correspondence between halfedges in
    new_halfedges and vertex positions in start_positions. So, you can write 
    loops of the form:

    for(size_t i = 0; i < new_halfedges.size(); i++)
    {
            Vec3 pi = start_positions[i]; // get the original vertex
            position corresponding to vertex i
    }
*/
void Halfedge_Mesh::bevel_face_positions(const std::vector<Vec3>& start_positions,
                                         Halfedge_Mesh::FaceRef face, float tangent_offset,
                                         float normal_offset) {

	// A2 Local(REQUIRED): bevel_face_positions
	
	std::vector<HalfedgeRef> new_halfedges;
	auto h = face->halfedge;
	do {
		new_halfedges.push_back(h);
		h = h->next;
	} while (h != face->halfedge);

	(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;
    (void)normal_offset;
}

/******************************************************************
*********************** Global Operations *************************
******************************************************************/

/*
    Splits all non-triangular faces into triangles.
*/
void Halfedge_Mesh::triangulate() {

	// A2 Global(REQUIRED): triangulate
	
	// For each face...
}

/*
    Sub-divide each face by creating edges from its cetroid to its edges
*/
void Halfedge_Mesh::linear_subdivide_positions() {

	// A2 Global(REQUIRED): linear_subdivide
	
	// For each vertex, assign Vertex::new_pos to
    // its original position, Vertex::pos.

    // For each edge, assign the midpoint of the two original
    // positions to Edge::new_pos.

    // For each face, assign the centroid (i.e., arithmetic mean)
    // of the original vertex positions to Face::new_pos. Note
    // that in general, NOT all faces will be triangles!
}

/*
    Sub-divide each face using new vertex posistions calucluated using the Catmull-Clark ruleset
*/
void Halfedge_Mesh::catmullclark_subdivide_positions() {

	// A2 Global(REQUIRED): catmullclark_subdivide

	// The implementation for this routine should be
    // a lot like Halfedge_Mesh:linear_subdivide_positions:(),
    // except that the calculation of the positions themsevles is
    // slightly more involved, using the Catmull-Clark subdivision
    // rules. (These rules are outlined in the Developer Manual.)

    // Faces

    // Edges

    // Vertices
}

/*
    Sub-divide each face based on the Loop subdivision rule
*/
void Halfedge_Mesh::loop_subdivide() {

	// A2 Global: loop_subdivide
	// Reminder: Only one of {loop_subdivide, isotropic_remesh, simplify} is required!

	// Each vertex and edge of the original mesh can be associated with a
    // vertex in the new (subdivided) mesh.
    // Therefore, our strategy for computing the subdivided vertex locations is to
    // *first* compute the new positions
    // using the connectivity of the original (coarse) mesh. Navigating this mesh
    // will be much easier than navigating
    // the new subdivided (fine) mesh, which has more elements to traverse.  We
    // will then assign vertex positions in
    // the new mesh based on the values we computed for the original mesh.
    
    // Compute new positions for all the vertices in the input mesh using
    // the Loop subdivision rule and store them in Vertex::new_pos.
    //    At this point, we also want to mark each vertex as being a vertex of the
    //    original mesh. Use Vertex::is_new for this.
    
    // Next, compute the subdivided vertex positions associated with edges, and
    // store them in Edge::new_pos.
    
    // Next, we're going to split every edge in the mesh, in any order.
    // We're also going to distinguish subdivided edges that came from splitting 
    // an edge in the original mesh from new edges by setting the boolean Edge::is_new. 
    // Note that in this loop, we only want to iterate over edges of the original mesh.
    // Otherwise, we'll end up splitting edges that we just split (and the
    // loop will never end!)
    
    // Now flip any new edge that connects an old and new vertex.
    
    // Finally, copy new vertex positions into the Vertex::pos.
}

/*
    Isotropic remeshing
*/
bool Halfedge_Mesh::isotropic_remesh(uint32_t outer_iterations, uint32_t smoothing_iterations) {

	// A2 Global: isotropic_remesh
	// Reminder: Only one of {loop_subdivide, isotropic_remesh, simplify} is required!

  	// Compute the mean edge length.
    // Repeat the four main steps for 5 or 6 iterations
    // -> Split edges much longer than the target length (being careful about
    //    how the loop is written!)
    // -> Collapse edges much shorter than the target length.  Here we need to
    //    be EXTRA careful about advancing the loop, because many edges may have
    //    been destroyed by a collapse (which ones?)
    // -> Now flip each edge if it improves vertex degree
    // -> Finally, apply some tangential smoothing to the vertex positions

    // Note: if you erase elements in a local operation, they will not be actually deleted
    // until do_erase or validate is called. This is to facilitate checking
    // for dangling references to elements that will be erased.
    // The rest of the codebase will automatically call validate() after each op,
    // but here simply calling collapse_edge() will not erase the elements.
    // You should use collapse_edge_erase() instead for the desired behavior.

	return false;
}

struct Edge_Record {
	Edge_Record() {
	}
	Edge_Record(std::unordered_map<uint32_t, Mat4>& VQ, Halfedge_Mesh::EdgeRef e) : edge(e) {
		
		// Compute the combined quadric from the edge endpoints.
        // -> Build the 3x3 linear system whose solution minimizes the quadric error
        //    associated with these two endpoints.
        // -> Use this system to solve for the optimal position, and store it in
        //    Edge_Record::optimal.
        // -> Also store the cost associated with collapsing this edge in
        //    Edge_Record::cost.
	}
	Halfedge_Mesh::EdgeRef edge;
	Vec3 optimal;
	float score;
};

bool operator<(const Edge_Record& r1, const Edge_Record& r2) {
	if (r1.score != r2.score) {
		return (r1.score < r2.score);
	}
	Halfedge_Mesh::EdgeRef e1 = r1.edge;
	Halfedge_Mesh::EdgeRef e2 = r2.edge;
	return &*e1 < &*e2;
}

template<class T> struct MutablePriorityQueue {
	void insert(const T& item) {
		queue.insert(item);
	}
	void remove(const T& item) {
		if (queue.find(item) != queue.end()) {
			queue.erase(item);
		}
	}
	const T& top() const {
		return *(queue.begin());
	}
	void pop() {
		queue.erase(queue.begin());
	}
	size_t size() {
		return queue.size();
	}

	std::set<T> queue;
};

/*
    Mesh simplification
*/
bool Halfedge_Mesh::simplify(float ratio) {

	// A2 Global: simplify
	// Reminder: Only one of {loop_subdivide, isotropic_remesh, simplify} is required!

	std::unordered_map<uint32_t, Mat4> VQ;
	std::unordered_map<uint32_t, Mat4> FQ;
	std::unordered_map<uint32_t, Edge_Record> ER;
	MutablePriorityQueue<Edge_Record> queue;

	// Compute initial quadrics for each face by simply writing the plane equation
    // for the face in homogeneous coordinates. These quadrics should be stored
    // in face_quadrics
    // -> Compute an initial quadric for each vertex as the sum of the quadrics
    //    associated with the incident faces, storing it in vertex_quadrics
    // -> Build a priority queue of edges according to their quadric error cost,
    //    i.e., by building an Edge_Record for each edge and sticking it in the
    //    queue. You may want to use the above PQueue<Edge_Record> for this.
    // -> Until we reach the target edge budget, collapse the best edge. Remember
    //    to remove from the queue any edge that touches the collapsing edge
    //    BEFORE it gets collapsed, and add back into the queue any edge touching
    //    the collapsed vertex AFTER it's been collapsed. Also remember to assign
    //    a quadric to the collapsed vertex, and to pop the collapsed edge off the
    //    top of the queue.

    // Note: if you erase elements in a local operation, they will not be actually deleted
    // until do_erase or validate are called. This is to facilitate checking
    // for dangling references to elements that will be erased.
    // The rest of the codebase will automatically call validate() after each op,
    // but here simply calling collapse_edge() will not erase the elements.
    // You should use collapse_edge_erase() instead for the desired behavior.

    return false;
}
