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

#include "../lib/mathlib.h"

// Types of sub-division
enum class SubD : uint8_t { linear, catmull_clark, loop };

class Indexed_Mesh;

class Halfedge_Mesh {
public:
	/*
	    For code clarity, we often want to distinguish between
	    an integer that encodes an index (an "ordinal" number)
	    from an integer that encodes a size (a "cardinal" number).
	*/
	using Index = size_t;
	using Size = size_t;

	/*
	    A Halfedge_Mesh is comprised of four atomic element types:
	    vertices, edges, faces, and halfedges.
	*/
	class Vertex;
	class Edge;
	class Face;
	class Halfedge;

	/*
	    Rather than using raw pointers to mesh elements, we store references
	    as STL::iterators---for convenience, we give shorter names to these
	    iterators (e.g., EdgeRef instead of list<Edge>::iterator).
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
	// Student Local Operations | student/meshedit.cpp
	//////////////////////////////////////////////////////////////////////////////////////////

	// Note: if you erase elements in these methods, they will not be erased from the
	// element lists until do_erase or validate are called. This is to facilitate checking
	// for dangling references to elements that will be erased.
	// The rest of the codebase will automatically call validate() after each op,
	// but you may need to be aware of this when implementing global ops.
	// Specifically, when you need to collapse an edge in iostropic_remesh() or simplify(),
	// you should call collapse_edge_erase() instead of collapse_edge()

	/*
	    Merge all faces incident on a given vertex, returning a
	    pointer to the merged face.
	*/
	std::optional<FaceRef> erase_vertex(VertexRef v);

	/*
	    Merge the two faces on either side of an edge, returning a
	    pointer to the merged face.
	*/
	std::optional<FaceRef> erase_edge(EdgeRef e);

	/*
	    Collapse an edge, returning a pointer to the collapsed vertex
	*/
	std::optional<VertexRef> collapse_edge(EdgeRef e);

	/*
	    Collapse a face, returning a pointer to the collapsed vertex
	*/
	std::optional<VertexRef> collapse_face(FaceRef f);

	/*
	   Insets a vertex into the given face, returning a pointer to the new center
   */
	std::optional<VertexRef> inset_vertex(FaceRef f);

	/*
	    Flip an edge, returning a pointer to the flipped edge
	*/
	std::optional<EdgeRef> flip_edge(EdgeRef e);

	/*
	    Split an edge, returning a pointer to the inserted midpoint vertex; the
	    halfedge of this vertex should refer to one of the edges in the original
	    mesh
	*/
	std::optional<VertexRef> split_edge(EdgeRef e);

	/*
	    Bisect an edge, returning a pointer to the new inserted midpoint vertex
	*/
	std::optional<VertexRef> bisect_edge(EdgeRef e);

	/*
	    Creates a face in place of the vertex, returning a pointer to the new face
	*/
	std::optional<FaceRef> bevel_vertex(VertexRef v);

	/*
	    Creates a face in place of the edge, returning a pointer to the new face
	*/
	std::optional<FaceRef> bevel_edge(EdgeRef e);

	/*
	    Insets a face into the given face, returning a pointer to the new center face
	*/
	std::optional<FaceRef> bevel_face(FaceRef f);

	/*
	    Computes vertex positions for a face that was just created by beveling a vertex,
	    but not yet confirmed.
	*/
	void bevel_vertex_positions(const std::vector<Vec3>& start_positions, FaceRef face,
	                            float tangent_offset);

	/*
	    Computes vertex positions for a face that was just created by beveling an edge,
	    but not yet confirmed.
	*/
	void bevel_edge_positions(const std::vector<Vec3>& start_positions, FaceRef face,
	                          float tangent_offset);

	/*
	    Computes vertex positions for a face that was just created by beveling a face,
	    but not yet confirmed.
	*/
	void bevel_face_positions(const std::vector<Vec3>& start_positions, FaceRef face,
	                          float tangent_offset, float normal_offset);

	/*
	    Collapse an edge, returning a pointer to the collapsed vertex
	    ** Also deletes the erased elements **
	*/
	std::optional<VertexRef> collapse_edge_erase(EdgeRef e) {
		auto r = collapse_edge(e);
		do_erase();
		return r;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// Student Global Operations | student/meshedit.cpp
	//////////////////////////////////////////////////////////////////////////////////////////

	/*
	    Splits all non-triangular faces into triangles.
	*/
	void triangulate();

	/*
	    Compute new vertex positions for a mesh that splits each polygon
	    into quads (by inserting a vertex at the face midpoint and each
	    of the edge midpoints).  The new vertex positions will be stored
	    in the members Vertex::new_pos, Edge::new_pos, and
	    Face::new_pos.  The values of the positions are based on
	    simple linear interpolation, e.g., the edge midpoints and face
	    centroids.
	*/
	void linear_subdivide_positions();

	/*
	    Compute new vertex positions for a mesh that splits each polygon
	    into quads (by inserting a vertex at the face midpoint and each
	    of the edge midpoints).  The new vertex positions will be stored
	    in the members Vertex::new_pos, Edge::new_pos, and
	    Face::new_pos.  The values of the positions are based on
	    the Catmull-Clark rules for subdivision.
	*/
	void catmullclark_subdivide_positions();

	/*
	    Sub-divide each face based on the Loop subdivision rule
	*/
	void loop_subdivide();

	/*
	    Isotropic remeshing
	*/
	bool isotropic_remesh(uint32_t outer_iterations, uint32_t smoothing_iterations);

	/*
	    Mesh simplification
	*/
	bool simplify(float ratio);

	//////////////////////////////////////////////////////////////////////////////////////////
	// End student operations, begin methods students should use
	//////////////////////////////////////////////////////////////////////////////////////////

	class Vertex {
	public:
		//connectivity:
		HalfedgeRef halfedge; //a halfedge that starts at this vertex

		//data:
		Vec3 position; //location of the vertex
		uint32_t id; //unique-within-the-mesh ID for this vertex

		struct Bone_Weight {
			uint32_t bone;
			float weight;
		};
		std::vector< Bone_Weight > bone_weights; //how much this vertex follows each bone's transform (used for skinned meshes)

		// Returns whether the vertex is on a boundary loop (TODO: needed?)
		bool on_boundary() const;

		// Returns the number of faces adjacent to the vertex, including boundary faces
		uint32_t degree() const;

		// Computes an area-weighted normal vector at the vertex (excluding any boundary faces)
		Vec3 normal() const;

		// Returns the position of the vertex (TODO: named 'center' for ... reasons ...)
		Vec3 center() const { return position; }

		// Returns the average of neighboring vertex positions:
		Vec3 neighborhood_center() const;

		float angle_defect() const;
		float gaussian_curvature() const;

	private:
		Vertex(uint32_t id_) : id(id_) {
		}
		Vec3 new_pos;
		bool is_new = false;
		friend class Halfedge_Mesh;
	};

	class Edge {
	public:
		//connectivity:
		HalfedgeRef halfedge; //one of the two halfedges adjacent to this edge

		//data:
		uint32_t id; //unique-in-this-mesh id
		bool sharp = false; //should this edge be considered sharp when computing shading normals?

		// Returns whether this edge is adjacent to a boundary face:
		bool on_boundary() const;
		// Returns the center point of the edge
		Vec3 center() const;
		// Returns the average of the face normals on either side of this edge
		Vec3 normal() const;
		// Returns the length of the edge
		float length() const;

	private:
		Edge(uint32_t id_, bool sharp_) : id(id_), sharp(sharp_) {
		}
		Vec3 new_pos;
		bool is_new = false;
		HalfedgeRef _halfedge;
		friend class Halfedge_Mesh;
	};

	class Face {
	public:
		//connectivity:
		HalfedgeRef halfedge; // some halfedge in this face

		//data:
		uint32_t id; //unique-in-this-mesh id
		bool boundary = false; //is this a boundary loop?

		// Returns the centroid of this face
		Vec3 center() const;
		// Returns an area weighted face normal
		Vec3 normal() const;
		// Returns the number of vertices/edges in this face
		uint32_t degree() const;
		// Returns area of this face:
		float area() const;

	private:
		Face(uint32_t id_, bool boundary_) : id(id_), boundary(boundary_) {
		}
		Vec3 new_pos;
		friend class Halfedge_Mesh;
	};

	class Halfedge {
	public:
		//connectivity:
		HalfedgeRef twin, next;
		VertexRef vertex;
		EdgeRef edge;
		FaceRef face;

		//data:
		//both uvs and shading normals may be different for different faces that meet at the same vertex,
		// so this data is stored on halfedges instead of (say) vertices:
		Vec2 corner_uv = Vec2(0.0f, 0.0f); //uv coordinate for this corner of the face
		Vec3 corner_normal = Vec3(0.0f, 0.0f, 0.0f); //shading normal for this corner of the face

		//unique-in-this-mesh id:
		uint32_t id;

		// Returns whether this edge is inside a boundary face
		bool is_boundary() const { return face->boundary; }

		// Convenience function for setting all members of the halfedge
		void set_neighbors(HalfedgeRef next_, HalfedgeRef twin_, VertexRef vertex_, EdgeRef edge_,
		                   FaceRef face_) {
			next = next_;
			twin = twin_;
			vertex = vertex_;
			edge = edge_;
			face = face_;
		}

	private:
		Halfedge(uint32_t id_) : id(id_) {
		}
		friend class Halfedge_Mesh;
	};

	/*
	    These methods delete a specified mesh element. One should think very, very carefully
	    about exactly when and how to delete mesh elements, since other elements will often still
	    point to the element that is being deleted, and accessing a deleted element will cause your
	    program to crash (or worse!). A good exercise to think about is: suppose you're
	    iterating over a linked list, and want to delete some of the elements as you go. How do you
	    do this without causing any problems? For instance, if you delete the current element, will
	    you be able to iterate to the next element?  Etc.

	    Note: the elements are not actually deleted until validate() is called in order to
	   facilitate checking for dangling references.
	*/
	void erase(VertexRef v) {
		verased.insert(v);
	}
	void erase(EdgeRef e) {
		eerased.insert(e);
	}
	void erase(FaceRef f) {
		ferased.insert(f);
	}
	void erase(HalfedgeRef h) {
		herased.insert(h);
	}

	/*
	    These methods allocate new mesh elements, returning a pointer (i.e., iterator) to the
	    new element. (These methods cannot have const versions, because they modify the mesh!)
	*/
	HalfedgeRef new_halfedge() {
		HalfedgeRef halfedge = halfedges.insert(halfedges.end(), Halfedge(next_id++));
		halfedge->twin = halfedges.end();
		halfedge->next = halfedges.end();
		halfedge->vertex = vertices.end();
		halfedge->edge = edges.end();
		halfedge->face = faces.end();
		return halfedge;
	}
	VertexRef new_vertex() {
		VertexRef vertex = vertices.insert(vertices.end(), Vertex(next_id++));
		vertex->halfedge = halfedges.end();
		return vertex;
	}
	EdgeRef new_edge(bool sharp = false) {
		EdgeRef edge = edges.emplace(edges.end(), Edge(next_id++, sharp));
		edge->halfedge = halfedges.end();
		return edge;
	}
	FaceRef new_face(bool boundary = false) {
		FaceRef face = faces.insert(faces.end(), Face(next_id++, boundary));
		face->halfedge = halfedges.end();
		return face;
	}

	/*
	    These methods return iterators to the beginning and end of the lists of
	    each type of mesh element.  For instance, to iterate over all vertices
	    one can write

	        for(VertexRef v = vertices_begin(); v != vertices_end(); v++)
	        {
	            // do something interesting with v
	        }

	    Note that we have both const and non-const versions of these functions;when
	    a mesh is passed as a constant reference, we would instead write

	        for(VertexCRef v = ...)

	    rather than VertexRef.
	*/
	HalfedgeRef halfedges_begin() {
		return halfedges.begin();
	}
	HalfedgeCRef halfedges_begin() const {
		return halfedges.begin();
	}
	HalfedgeRef halfedges_end() {
		return halfedges.end();
	}
	HalfedgeCRef halfedges_end() const {
		return halfedges.end();
	}
	VertexRef vertices_begin() {
		return vertices.begin();
	}
	VertexCRef vertices_begin() const {
		return vertices.begin();
	}
	VertexRef vertices_end() {
		return vertices.end();
	}
	VertexCRef vertices_end() const {
		return vertices.end();
	}
	EdgeRef edges_begin() {
		return edges.begin();
	}
	EdgeCRef edges_begin() const {
		return edges.begin();
	}
	EdgeRef edges_end() {
		return edges.end();
	}
	EdgeCRef edges_end() const {
		return edges.end();
	}
	FaceRef faces_begin() {
		return faces.begin();
	}
	FaceCRef faces_begin() const {
		return faces.begin();
	}
	FaceRef faces_end() {
		return faces.end();
	}
	FaceCRef faces_end() const {
		return faces.end();
	}

	/*
	    This return simple statistics about the current mesh.
	*/
	Size n_vertices() const {
		return vertices.size();
	};
	Size n_edges() const {
		return edges.size();
	};
	Size n_faces() const {
		return faces.size();
	};
	Size n_halfedges() const {
		return halfedges.size();
	};

	bool has_boundary() const;
	Size n_boundaries() const;

	/// Check if half-edge mesh is valid:
	// - references held by elements are to valid, non-deleted elements
	// - halfedges exist in twinned pairs
	// - the halfedges that reference a vertex are exactly those accessible by `vertex->halfedge(->twin->next)^n`
	// - the halfedges that reference an edge are exactly those accessible by `edge->halfedge(->twin)^n`
	// - the halfedges that reference a face are exactly those accessible by `face->halfedge(->next)^n`
	// - vertices are not orphaned (they have at least one non-boundary face adjacent)
	// - vertices are on at most one boundary face
	// - edges are not orphaned (they have at least one non-boundary face adjacent)
	// - faces are simple (touch each vertex / edge at most once)
	// - faces have at least three vertices
	std::optional<std::pair<ElementRef, std::string>> validate();

	//////////////////////////////////////////////////////////////////////////////////////////
	// End methods students should use, begin internal methods - you don't need to use these
	//////////////////////////////////////////////////////////////////////////////////////////

	Halfedge_Mesh() = default;
	~Halfedge_Mesh() = default;

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
	Halfedge_Mesh(const Halfedge_Mesh& src) = delete; //make a copy of the data in an existing mesh
	void operator=(const Halfedge_Mesh& src) = delete; //TODO: needed?

	/// Creates (TODO: new?) sub-divided mesh with provided scheme
	bool subdivide(SubD strategy);

	/// Re-orients every face in the mesh by re-setting halfedge next pointers.
	// (i.e., Halfedges remain on the same edge but point in the opposite direction.)
	// no elements are created or erased; only next pointers are changed.
	void flip_orientation();

	// set corner normals:
	// "smooth mode" (threshold >= 180.0f)
	//   - all edges treated as smooth (even if 'sharp' flag is set)
	// "auto mode" (0.0f < threshold < 180.0f)
	//   - all edges with sharp flag are treated as sharp.
	//   - all edges with angle (in degrees) > threshold are also treated as sharp.
	// "flat mode" (threshold <= 0)
	//   - all edges are treated as sharp
	void set_corner_normals(float threshold = 30.0f);

	// set corner uvs per-face:
	void set_corner_uvs_per_face();

	// set corner uvs by projection to a plane:
	//  origin maps to (0,0)
	//  origin + u_axis maps to (1,0)
	//  origin + v_axis maps to (0,1)
	void set_corner_uvs_project(Vec3 origin, Vec3 u_axis, Vec3 v_axis);

	/// WARNING: erased elements stay in the element lists until do_erase()
	/// or validate() are called
	void do_erase();

	// Computes furthest vertex distance from origin
	float radius() const;

	static Vec3 normal_of(ElementRef elem);
	static Vec3 center_of(ElementRef elem);
	static uint32_t id_of(ElementRef elem);

private:
	std::list<Vertex> vertices;
	std::list<Edge> edges;
	std::list<Face> faces;
	std::list<Halfedge> halfedges;

	uint32_t next_id = 0;

	std::set<VertexRef> verased;
	std::set<EdgeRef> eerased;
	std::set<FaceRef> ferased;
	std::set<HalfedgeRef> herased;

	friend class Test;
};

/*
    Some algorithms need to know how to compare two iterators (std::map)
    Here we just say that one iterator comes before another if the address of the
    object it points to is smaller. (You should not have to worry about this!)
*/
inline bool operator<(const Halfedge_Mesh::HalfedgeRef& i, const Halfedge_Mesh::HalfedgeRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::VertexRef& i, const Halfedge_Mesh::VertexRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::EdgeRef& i, const Halfedge_Mesh::EdgeRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::FaceRef& i, const Halfedge_Mesh::FaceRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::HalfedgeCRef& i, const Halfedge_Mesh::HalfedgeCRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::VertexCRef& i, const Halfedge_Mesh::VertexCRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::EdgeCRef& i, const Halfedge_Mesh::EdgeCRef& j) {
	return &*i < &*j;
}
inline bool operator<(const Halfedge_Mesh::FaceCRef& i, const Halfedge_Mesh::FaceCRef& j) {
	return &*i < &*j;
}

/*
    Some algorithms need to know how to hash references (std::unordered_map)
    Here we simply hash the unique ID of the element.
*/
namespace std {
template<> struct hash<Halfedge_Mesh::VertexRef> {
	uint64_t operator()(Halfedge_Mesh::VertexRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::EdgeRef> {
	uint64_t operator()(Halfedge_Mesh::EdgeRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::FaceRef> {
	uint64_t operator()(Halfedge_Mesh::FaceRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::HalfedgeRef> {
	uint64_t operator()(Halfedge_Mesh::HalfedgeRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::VertexCRef> {
	uint64_t operator()(Halfedge_Mesh::VertexCRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::EdgeCRef> {
	uint64_t operator()(Halfedge_Mesh::EdgeCRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::FaceCRef> {
	uint64_t operator()(Halfedge_Mesh::FaceCRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
template<> struct hash<Halfedge_Mesh::HalfedgeCRef> {
	uint64_t operator()(Halfedge_Mesh::HalfedgeCRef key) const {
		static const std::hash<uint32_t> h;
		return h(key->id);
	}
};
} // namespace std
