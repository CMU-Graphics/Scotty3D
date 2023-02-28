#pragma once

/*
 * Halfedge_Mesh
 *   Written By Keenan Crane for 15-462 Assignment 2.
 *   Updated by Max Slater for Fall 2020.
 *   Updated (extensively!) by Jim McCann for Fall 2022.
 *
 *  Represents an oriented, manifold 3D shape as a collection of vertices,
 *  halfedges, edges, and faces. Connectivity is pointer-based, allowing for
 *  many local operations to be done in constant time.
 *
 *  Here, manifold means that the neighborhood of every vertex is either
 *  equivalent to a disc (interior vertices) or a half-disc (boundary vertices).
 *
 *  Faces are oriented counterclockwise.
 *
 */

#include <list>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

#include "../lib/mathlib.h"

class Indexed_Mesh;

class Halfedge_Mesh {
public:
	/*
	 * A Halfedge_Mesh is comprised of four atomic element types:
	 * vertices, edges, faces, and halfedges.
	 */
	class Vertex;
	class Edge;
	class Face;
	class Halfedge;

	/*
	 * Rather than using raw pointers to mesh elements, we store references
	 * as iterators. For convenience, we give shorter names to these
	 * iterators (e.g., EdgeRef instead of list<Edge>::iterator).
	 */
	using VertexRef = std::list<Vertex>::iterator;
	using EdgeRef = std::list<Edge>::iterator;
	using FaceRef = std::list<Face>::iterator;
	using HalfedgeRef = std::list<Halfedge>::iterator;

	/* This is a special kind of reference that can refer to any of the four
	   element types. */
	using ElementRef = std::variant<VertexRef, EdgeRef, HalfedgeRef, FaceRef>;

	/*
	    We also need "const" iterator types, for situations where a method takes
	    a constant reference or pointer to a Halfedge_Mesh.  Since these types are
	    used so frequently, we will use "CRef" as a shorthand abbreviation for
	    "constant iterator."
	*/
	using VertexCRef = std::list<Vertex>::const_iterator;
	using EdgeCRef = std::list<Edge>::const_iterator;
	using FaceCRef = std::list<Face>::const_iterator;
	using HalfedgeCRef = std::list<Halfedge>::const_iterator;
	using ElementCRef = std::variant<VertexCRef, EdgeCRef, HalfedgeCRef, FaceCRef>;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Local Edit Operations | src/geometry/halfedge-local.cpp
	//////////////////////////////////////////////////////////////////////////////////////////

	//if the result of an operation would cause the mesh to become invalid,
	// it will instead not change the mesh and return std::nullopt.
	//otherwise it returns the created/modified element (unless otherwise noted)

	//--- creation ---

	//create regular polygon face, disconnected from anything else
	// (except an opposite-oriented boundary face required by validity)
	// face is centered at (0,0,0), has normal (0,0,1), and has first edge pointing in the +x direction
	std::optional< FaceRef > add_face(uint32_t sides, float radius);


	//--- division ---

	//add a new vertex in the center of an edge
	// (does not divide the adjacent faces)
	std::optional<VertexRef> bisect_edge(EdgeRef e);

	//add a vertex at the midpoint of an edge and divide the adjacent non-boundary faces
	// - the newly added vertex's halfedge should be aligned with the original edge
	std::optional<VertexRef> split_edge(EdgeRef e);

	//put a vertex in the center of a face; divide f into a triangle fan around the new vertex
	std::optional<VertexRef> inset_vertex(FaceRef f);

	//creates a face in place of a vertex, returning a reference to the new face
	std::optional<FaceRef> bevel_vertex(VertexRef v);

	//creates a face in place of an edge, returning a reference to the new face
	std::optional<FaceRef> bevel_edge(EdgeRef e);

	//inset a face into a face, return reference to the center face
	std::optional<FaceRef> extrude_face(FaceRef f);


	//--- modification ---

	//rotate non-boundary edge counter-clockwise
	std::optional<EdgeRef> flip_edge(EdgeRef e);

	//turn a non-boundary face into a boundary face
	std::optional<FaceRef> make_boundary(FaceRef f);


	//--- unification ---

	//merge non-boundary faces incident to a given vertex, return the resulting face
	std::optional<FaceRef> dissolve_vertex(VertexRef v);

	//merge faces incident on an edge, return the resulting face
	// (merging with a boundary face makes the resulting face boundary)
	std::optional<FaceRef> dissolve_edge(EdgeRef e);

	//remove an edge by collapsing it to a vertex at its midpoint
	std::optional<VertexRef> collapse_edge(EdgeRef e);

	//remove a face by collapsing it to a vertex at its centroid
	std::optional<VertexRef> collapse_face(FaceRef f);

	//weld two boundary edges together, returns first argument on success
	std::optional<EdgeRef> weld_edges(EdgeRef e, EdgeRef e2);


	//--- helpers ---

	//helpers used by the UI in conjunction with the above when doing beveling/extrusion operations

	//set vertex positions for a face created by beveling a vertex or edge
	void bevel_positions(FaceRef face, std::vector<Vec3> const & start_positions, Vec3 direction, float distance);

	//set vertex positions for a face created by extruding a face
	void extrude_positions(FaceRef face, Vec3 move, float shrink);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Global Edit Operations | src/geometry/halfedge-global.cpp
	//////////////////////////////////////////////////////////////////////////////////////////

	//--- student-implemented methods ---

	//split all non-triangular non-boundary faces into triangles
	void triangulate();

	//add new vertex at every face and edge center, without changing mesh shape
	void linear_subdivide();

	//add new vertex at every face and edge center, applying Catmull-Clark-style position updates
	void catmark_subdivide();

	//subdivide all non-boundary faces using the Loop subdivision rule.
	// if any non-boundary face is a triangle, does nothing and returns false
	// otherwise, returns true
	bool loop_subdivide();

	//improve mesh quality via isotropic remeshing
	struct Isotropic_Remesh_Parameters {
		uint32_t outer_iterations = 3; //how many outer loops through the remeshing process to take
		float longer_factor = 1.2f; //edges longer than longer_factor * target_length are split
		float shorter_factor = 0.8f; //edges shorter than shorter_factor * target_length are collapsed
		uint32_t smoothing_iterations = 10; //how many tangential smoothing iterations to run
		float smoothing_step = 0.2f; //amount to interpolate vertex positions toward their centroid each smoothing step
	};
	void isotropic_remesh(Isotropic_Remesh_Parameters const &params);

	//collapse edges until no more than ratio * |edges| remain
	// returns 'true' if it succeeds, 'false' if it ran out of collapse-able edges
	bool simplify(float ratio);

	//--- provided methods ---

	//helper for Catmull-Clark-style subdivision.
	// generates topology itself, assigns positions from parameters.
	// used by linear_subdivide and catmark_subdivide.
	void catmark_subdivide_helper(
		std::unordered_map< VertexCRef, Vec3 > const &vertex_positions, //where to move current vertices after subdividing
		std::unordered_map< EdgeCRef, Vec3 > const &edge_vertex_positions, //where to place the center-of-edge vertices
		std::unordered_map< FaceCRef, Vec3 > const &face_vertex_positions //where to place the center-of-face vertices
	);

	//re-orients every face in the mesh by flipping halfedge directions.
	// no elements are created or erased; only Halfedge::next, Halfedge::vertex, and Vertex::halfedge pointers are changed.
	void flip_orientation();

	//set corner normals:
	// "smooth mode" (threshold >= 180.0f)
	//   - all edges treated as smooth (even if 'sharp' flag is set)
	// "auto mode" (0.0f < threshold < 180.0f)
	//   - all edges with sharp flag are treated as sharp.
	//   - all edges with angle (in degrees) > threshold are also treated as sharp.
	// "flat mode" (threshold <= 0)
	//   - all edges are treated as sharp
	void set_corner_normals(float threshold = 30.0f);

	//set corner uvs per-face:
	void set_corner_uvs_per_face();

	//set corner uvs by projection to a plane:
	//  origin maps to (0,0)
	//  origin + u_axis maps to (1,0)
	//  origin + v_axis maps to (0,1)
	void set_corner_uvs_project(Vec3 origin, Vec3 u_axis, Vec3 v_axis);


	//////////////////////////////////////////////////////////////////////////////////////////
	// Element Definitions, Utility Functions | src/geometry/halfedge-utility.cpp
	//////////////////////////////////////////////////////////////////////////////////////////

	//--- mesh elements ---

	class Vertex {
	public:
		//connectivity:
		HalfedgeRef halfedge; //some halfedge that starts at this vertex

		//data:
		uint32_t id; //unique-within-the-mesh ID for this vertex
		Vec3 position; //location of the vertex
		//how much this vertex follows each bone's transform (used for skinned meshes):
		struct Bone_Weight {
			uint32_t bone;
			float weight;
		};
		std::vector< Bone_Weight > bone_weights;

		//helpers:
		bool on_boundary() const; //is vertex on a boundary loop?
		uint32_t degree() const; //number of faces adjacent to the vertex, including boundary faces
		Vec3 normal() const; //area-weighted normal vector at the vertex (excluding any boundary faces)
		Vec3 neighborhood_center() const; //center of adjacent vertices
		float angle_defect() const; //difference between sum of adjacent face angles and 2pi
		float gaussian_curvature() const; //product of principle curvatures

	private:
		Vertex(uint32_t id_) : id(id_) { }
		friend class Halfedge_Mesh;
	};

	class Edge {
	public:
		//connectivity:
		HalfedgeRef halfedge; //one of the two halfedges adjacent to this edge

		//data:
		uint32_t id; //unique-in-this-mesh id
		bool sharp = false; //should this edge be considered sharp when computing shading normals?

		//helpers:
		bool on_boundary() const; // Is edge on a boundary loop?
		Vec3 center() const; // The midpoint of the edge
		Vec3 normal() const; // The average of the face normals on either side of this edge
		float length() const; // The length of the edge

	private:
		Edge(uint32_t id_, bool sharp_) : id(id_), sharp(sharp_) { }
		friend class Halfedge_Mesh;
	};

	class Face {
	public:
		//connectivity:
		HalfedgeRef halfedge; // some halfedge in this face

		//data:
		uint32_t id; //unique-in-this-mesh id
		bool boundary = false; //is this a boundary loop?

		//helpers:
		Vec3 center() const; // centroid of this face
		Vec3 normal() const; // area-weighted face normal
		uint32_t degree() const; // number of vertices/edges in this face
		float area() const; // area of this face

	private:
		Face(uint32_t id_, bool boundary_) : id(id_), boundary(boundary_) { }
		friend class Halfedge_Mesh;
	};

	class Halfedge {
	public:
		//connectivity:
		HalfedgeRef twin; //halfedge on the other side of edge
		HalfedgeRef next; //next halfedge ccw around face
		VertexRef vertex; //vertex this halfedge is leaving
		EdgeRef edge; //edge this halfedge is half of
		FaceRef face; //face this halfedge borders

		//data:
		uint32_t id; //unique-in-this-mesh id
		//both uvs and shading normals may be different for different faces that meet at the same vertex,
		// so this data is stored on halfedges instead of (say) vertices:
		Vec2 corner_uv = Vec2(0.0f, 0.0f); //uv coordinate for this corner of the face
		Vec3 corner_normal = Vec3(0.0f, 0.0f, 0.0f); //shading normal for this corner of the face

		//helpers:
		//convenience function for setting connectivity
		void set_tnvef(HalfedgeRef twin_, HalfedgeRef next_, VertexRef vertex_, EdgeRef edge_, FaceRef face_) {
			twin = twin_; next = next_; vertex = vertex_; edge = edge_; face = face_;
		}


	private:
		Halfedge(uint32_t id_) : id(id_) { }
		friend class Halfedge_Mesh;
	};

	//--- data management ---
	
	//interpolate_data:
	// sets the extra data on an element reference by mixing together data
	// from a list of element references. Useful when merging, splitting,
	// collapsing, or adding elements.
	
	// VertexRef version sets bone_weights:
	static void interpolate_data(std::vector< VertexCRef > const &from, VertexRef to);
	// HalfedgeRef version sets corner_uv and corner_normal:
	static void interpolate_data(std::vector< HalfedgeCRef > const &from, HalfedgeRef to);

	/* interpolate_data usage examples:
	  //TODO: change this and explain it a bit more

	  if adding a vertex vm on an edge with vertices (v1, v2), call:
	    interpolate_data({v1, v2}, vm)
	  if adding a vertex vm in a face with vertices (v1, ..., vN) call:
	    interpolate_data({v1, ..., vN}, vm)
	  if merging vertices (v1, ..., vN) to vertex vm call:
	    interpolate_data({v1, ..., vN}, vm)

	  if dividing a halfedge h -> (h, h2), call:
	    interpolate_data({h, h->next}, h2)
	  if adding a halfedge h from the midpoint of a face with halfedges (h1, ..., hN) call:
	    interpolate_data({h1, ..., hN}, h2)
	  if merging halfedges (h1,h2) -> h call:
	    interpolate_data({h1, h2}, h)
	*/

	//--- memory management ---

	//use emplace_* to allocate new mesh elements:
	// elements are added at the end of their respective list
	// elements have a fresh id value and all references to other elements set to the end() of the appropriate list
	VertexRef emplace_vertex();
	EdgeRef emplace_edge(bool sharp = false);
	FaceRef emplace_face(bool boundary = false);
	HalfedgeRef emplace_halfedge();

	//use erase to delete mesh elements:
	// immediately clears the connectivity and data,
	// sets the high-order bit of their id to 1,
	// and adds to a free list for later reuse by emplace_*
	void erase_vertex(VertexRef v);
	void erase_edge(EdgeRef e);
	void erase_face(FaceRef f);
	void erase_halfedge(HalfedgeRef h);

	//elements are held in these lists:
	//Don't add/erase elements from these lists directly!
	// Use the emplace and erase functions above.
	std::list<Vertex> vertices;
	std::list<Edge> edges;
	std::list<Face> faces;
	std::list<Halfedge> halfedges;

	/* element lists usage example:

	  Visit every vertex in the mesh:
	    for (VertexRef v = vertices.begin(); v != vertices.end(); ++v) {
	        //do something for every vertex
	    }
	*/

	//--- misc stats ---

	//count boundaries
	size_t n_boundaries() const;

	//furthest vertex distance from origin
	float radius() const;

	//a multi-line description of the mesh, suitable for debug output (works on invalid meshes)
	std::string describe() const;

	//--- validation ---

	/// Check if half-edge mesh is valid:
	// - all references held by elements are to members of the vertices, edges, faces, or halfedges lists
	//   (no references to elements in other meshes, deleted elements, or past-the-end iterators)
	// - `edge->halfedge(->twin)^n` is a cycle of two halfedges
	//   - this is also exactly the set of halfedges that reference `edge`
	// - `face->halfedge(->next)^n` is a cycle of at least three halfedges
	//   - this is also exactly the set of halfedges that reference `face`
	// - `vertex->halfedge(->twin->next)^n` is a cycle of at least two halfedges
	//   - this is also exactly the set of halfedges that reference `vertex`
	// - vertices are not orphaned (they have at least one non-boundary face adjacent)
	// - vertices are on at most one boundary face
	// - edges are not orphaned (they have at least one non-boundary face adjacent)
	// - faces are simple (touch each vertex / edge at most once)
	// - data stored on elements is valid (i.e., non-infinite, non-NaN) (this is a relatively weak condition)
	std::optional<std::pair<ElementCRef, std::string>> validate() const;




	//////////////////////////////////////////////////////////////////////////////////////////
	// Internal Methods -- you don't need to use these
	//////////////////////////////////////////////////////////////////////////////////////////

	Halfedge_Mesh() = default;
	~Halfedge_Mesh() = default;

	using Index = uint32_t; //to distinguish indices from sizes

	//convert from raw polygon list or indexed mesh:
	// - mesh must correspond to a valid halfedge mesh (see 'validate()')
	// - vertices and faces will match the order they appear in vertices[] and faces[], respectively
	// - halfedges will match the order they are mentioned in faces[]
	static Halfedge_Mesh from_indexed_faces(std::vector< Vec3 > const &vertices, std::vector< std::vector< Index > > const &faces);
	static Halfedge_Mesh from_indexed_mesh(const Indexed_Mesh& mesh);

	//generate a cube:
	static Halfedge_Mesh cube(float cube_radius);

	//DEBUG: so test cases compile -- but this is going away:
	//TODO: remove this constructor:
	Halfedge_Mesh(std::vector< std::vector< Index > > const &faces, std::vector< Vec3 > const &vertices) {
		*this = from_indexed_faces(vertices, faces);
	}

	// You can create a mesh by moving data from an existing mesh:
	//TODO: needed?
	Halfedge_Mesh(Halfedge_Mesh&& src) = default; //take ownership of contents of existing mesh
	Halfedge_Mesh& operator=(Halfedge_Mesh&& src) = default;

	// You can create a mesh by getting a copy of an existing mesh:
	Halfedge_Mesh copy() const;
	// these are deleted to avoid implicit copies:
	Halfedge_Mesh(const Halfedge_Mesh& src) = delete;
	void operator=(const Halfedge_Mesh& src) = delete;

	
	//--- generic element helpers used by the gui ---


	// get properties of generic elements (used by GUI):
	static uint32_t id_of(ElementCRef elem);
	static Vec3 normal_of(ElementCRef elem);
	static Vec3 center_of(ElementCRef elem);

	static uint32_t id_of(ElementRef elem);
	static Vec3 normal_of(ElementRef elem);
	static Vec3 center_of(ElementRef elem);

	//helper for when you have an element reference and want a const element reference:
	static ElementCRef const_from(ElementRef elem);

private:
	
	//a fresh element id; assigned + incremented by emplace_*() functions:
	uint32_t next_id = 0;

	//free lists used by the erase() and emplace_*() functions:
	std::list<Vertex> free_vertices;
	std::list<Edge> free_edges;
	std::list<Face> free_faces;
	std::list<Halfedge> free_halfedges;

	friend class Test;
};



/*
    Some containers need to know how to compare two iterators (std::map)
    Here we just say that one iterator comes before another if the address of the
    object it points to is smaller. (You should not have to worry about this!)
*/
#define COMPARE_BY_ADDRESS(Ref) \
	inline bool operator<(Halfedge_Mesh::Ref const & i, Halfedge_Mesh::Ref const & j) { \
		return &*i < &*j; \
	}
COMPARE_BY_ADDRESS(HalfedgeRef);
COMPARE_BY_ADDRESS(VertexRef);
COMPARE_BY_ADDRESS(EdgeRef);
COMPARE_BY_ADDRESS(FaceRef);
COMPARE_BY_ADDRESS(HalfedgeCRef);
COMPARE_BY_ADDRESS(VertexCRef);
COMPARE_BY_ADDRESS(EdgeCRef);
COMPARE_BY_ADDRESS(FaceCRef);
#undef COMPARE_BY_ADDRESS

/*
	Some containers need to know how to hash references (std::unordered_map).
	Here we simply hash the address of the element, since these are stable.
*/
namespace std {
	#define HASH_BY_ADDRESS( T ) \
		template<> struct hash< T > { \
			uint64_t operator()(T const & key) const { \
				static const std::hash<decltype(&*key)> h; \
				return h(&*key); \
			} \
		}
	HASH_BY_ADDRESS( Halfedge_Mesh::VertexRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::EdgeRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::FaceRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::HalfedgeRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::VertexCRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::EdgeCRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::FaceCRef );
	HASH_BY_ADDRESS( Halfedge_Mesh::HalfedgeCRef );
	#undef HASH_BY_ADDRESS
} // namespace std
