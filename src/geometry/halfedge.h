/*
    Halfedge_Mesh.h

    Written By Keenan Crane for 15-462 Assignment 2.
    Updated by Max Slater for Fall 2020.
*/

/*
    A Halfedge_Mesh is a data structure that makes it easy to iterate over (and
    modify) a polygonal mesh.  The basic idea is that each edge of the mesh
    gets associated with two "halfedges," one on either side, that point in
    opposite directions.  These halfedges essentially serve as the "glue"
    between different mesh elements (vertices, edges, and faces).  A half edge
    mesh has the same basic flavor as a tree or linked list data structure:
    each node has pointers that reference other nodes.  In particular, each
    half edge points to:

        -its root vertex,
        -its associated edge,
        -the face it sits on,
        -its "twin", i.e., the halfedge on the other side of the edge,
        -and the next halfedge in cyclic order around the face.

    Vertices, edges, and faces each point to just one of their incident
    halfedges.  For instance, an edge will point arbitrarily to either
    its "left" or "right" halfedge.  Each vertex will point to one of
    many halfedges leaving that vertex.  Each face will point to one of
    many halfedges going around that face.  The fact that these choices
    are arbitrary does not at all affect the practical use of this data
    structure: they merely provide a starting point for iterating over
    the local region (e.g., walking around a face, or visiting the
    neighbors of a vertex).  A practical example of iterating around a
    face might look like:

        HalfEdgeRef h = face->halfedge();
        do {
                // do something interesting with h
                h = h->next();
        } while(h != face->halfEdge());

    At each iteration we walk to the "next" halfedge, until we return
    to the original starting point.  A slightly more interesting
    example is iterating around a vertex:

        HalfEdgeRef h = vert->halfedge();
        do {
                // do something interesting with h
                h = h->twin()->next();
        } while(h != vert->halfedge());

    (Can you draw a picture that explains this iteration?)  A very
    different kind of iteration is when we want to iterate over, say,
    all* the edges of a mesh:

        for(EdgeRef e = edges_begin(); e != edges_end(); e++) {
                // do something interesting with e
        }

    A very important consequence of the halfedge representation is that
    ---by design---it can only represent manifold, orientable triangle
    meshes.  I.e., every point should have a neighborhood that looks disk-
    like, and you should be able to assign to each polygon a normal
    direction such that all these normals "point the same way" as you walk
    around the surface.

    At a high level, that's all there is to know about the half edge
    data structure.  But it's worth making a few comments about how this
    particular implementation works---especially how things like boundaries
    are handled.  First and foremost, the "pointers" used in this
    implementation are actually STL iterators.  STL stands for the "standard
    template library," and is a basic part of C++ that provides some very
    convenient and powerful data structures and algorithms---if you've never
    looked at the STL before, now would be a great time to get familiar!  At
    a high level, STL iterators behave a lot like pointers: they don't store
    data, but rather reference some data that is allocated elsewhere.  And
    the syntax is also very similar; for instance, if p is an iterator, then
    *p yields the value referred to by p.  (As for the rest, Google is a
    terrific resource! :-))

    Rather than accessing raw iterators, the Halfedge_Mesh encapsulates these
    pointers using methods like Halfedge::twin(), Halfedge::next(), etc.  The
    reason for this encapsulation (as in most object-oriented programming)
    is that it allows the user to make changes to the internal representation
    later down the line.  For instance, if you know that the connectivity of
    the mesh is never going to change, you might be able to improve performance
    by (internally) replacing the linked lists with fixed-length arrays,
    without breaking any code that might have been written using the abstract
    interface.  (There are deeper reasons for this kind of encapsulation
    when working with polygon meshes, but that's a story for another time!)

    Finally, some surfaces have "boundary loops," e.g., a pair of pants has
    three boundaries: one at the waist, and two at the ankles.  These boundaries
    are represented by special faces in our halfedge mesh. Each
    face (boundary or regular) also stored a flag Face::boundary that
    indicates whether or not it is a boundary.  This value can be queried via the
    public method Face::is_boundary() (again: encapsulation!)  So for instance, if
    I wanted to know the area of all polygons that touch a given vertex, I might
    write some code like this:

        float totalArea = 0.0f;
        HalfEdgeRef h = vert->halfedge();
        do {
                // don't add the area of boundary faces!
                if(!h->face()->is_boundary()) {
                    totalArea += h->face()->area();
                }
                h = h->twin()->next();
        }
        while(h != vert->halfedge());

    In other words, whenever I'm processing a face, I should stop and ask: is
    this really a geometric face in my mesh?  Or is it just a "virtual" face
    that represents a boundary loop?  Finally, for convenience, the halfedge
    associated with a boundary vertex is the first halfedge on the boundary.
    In other words, if we want to iterate over, say, all faces touching a
    boundary vertex, we could write

        HalfEdgeRef h = boundary_vertex->halfedge();
        do {
                // do something interesting with h
                h = h->twin()->next();
        } while(!h->on_boundary());

    (Notice that this loop will never terminate for an interior vertex!)
    More documentation can be found in the inline comments below.
*/

#pragma once

#include <list>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "../platform/gl.h"

// Types of sub-division
enum class SubD { linear, catmullclark, loop };

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
        used so frequently, we will use "CIter" as a shorthand abbreviation for
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
    bool isotropic_remesh();

    /*
        Mesh simplification
    */
    bool simplify();

    //////////////////////////////////////////////////////////////////////////////////////////
    // End student operations, begin methods students should use
    //////////////////////////////////////////////////////////////////////////////////////////

    class Vertex {
    public:
        // Returns a halfedge incident from the vertex
        HalfedgeRef& halfedge() {
            return _halfedge;
        }
        HalfedgeCRef halfedge() const {
            return _halfedge;
        }

        // Returns whether the vertex is on a boundary loop
        bool on_boundary() const;
        // Returns the number of edges incident from this vertex
        unsigned int degree() const;
        // Computes an area-weighted normal vector at the vertex
        Vec3 normal() const;
        // Returns the position of the vertex
        Vec3 center() const;
        // Computes the centroid of the loop of the vertex
        Vec3 neighborhood_center() const;
        // Returns an id unique to this vertex
        unsigned int id() const {
            return _id;
        }

        // The vertex position
        Vec3 pos;

    private:
        Vertex(unsigned int id) : _id(id) {
        }
        Vec3 new_pos;
        bool is_new = false;
        unsigned int _id = 0;
        HalfedgeRef _halfedge;
        friend class Halfedge_Mesh;
    };

    class Edge {
    public:
        // Returns one of the two halfedges associated with this edge
        HalfedgeRef& halfedge() {
            return _halfedge;
        }
        HalfedgeCRef halfedge() const {
            return _halfedge;
        }

        // Returns whether this edge is contained in a boundary loop
        bool on_boundary() const;
        // Returns the center point of the edge
        Vec3 center() const;
        // Returns the average of the face normals on either side of this edge
        Vec3 normal() const;
        // Returns the length of the edge
        float length() const;
        // Returns an id unique to this edge
        unsigned int id() const {
            return _id;
        }

    private:
        Edge(unsigned int id) : _id(id) {
        }
        Vec3 new_pos;
        bool is_new = false;
        unsigned int _id = 0;
        HalfedgeRef _halfedge;
        friend class Halfedge_Mesh;
    };

    class Face {
    public:
        // Returns some halfedge contained within this face
        HalfedgeRef& halfedge() {
            return _halfedge;
        }
        HalfedgeCRef halfedge() const {
            return _halfedge;
        }

        // Returns whether this is a boundary face
        bool is_boundary() const {
            return boundary;
        }
        // Returns the centroid of this face
        Vec3 center() const;
        // Returns an area weighted face normal
        Vec3 normal() const;
        // Returns the number of vertices/edges in this face
        unsigned int degree() const;
        // Returns an id unique to this face
        unsigned int id() const {
            return _id;
        }

    private:
        Face(unsigned int id, bool is_boundary) : _id(id), boundary(is_boundary) {
        }
        Vec3 new_pos;
        unsigned int _id = 0;
        HalfedgeRef _halfedge;
        bool boundary = false;
        friend class Halfedge_Mesh;
    };

    class Halfedge {
    public:
        // Retrives the twin halfedge
        HalfedgeRef& twin() {
            return _twin;
        }
        HalfedgeCRef twin() const {
            return _twin;
        }

        // Retrieves the next halfedge
        HalfedgeRef& next() {
            return _next;
        }
        HalfedgeCRef next() const {
            return _next;
        }

        // Retrieves the associated vertex
        VertexRef& vertex() {
            return _vertex;
        }
        VertexCRef vertex() const {
            return _vertex;
        }

        // Retrieves the associated edge
        EdgeRef& edge() {
            return _edge;
        }
        EdgeCRef edge() const {
            return _edge;
        }

        // Retrieves the associated face
        FaceRef& face() {
            return _face;
        }
        FaceCRef face() const {
            return _face;
        }

        // Returns an id unique to this halfedge
        unsigned int id() const {
            return _id;
        }

        // Returns whether this edge is inside a boundary face
        bool is_boundary() const {
            return _face->is_boundary();
        }

        // Convenience function for setting all members of the halfedge
        void set_neighbors(HalfedgeRef next, HalfedgeRef twin, VertexRef vertex, EdgeRef edge,
                           FaceRef face) {
            _next = next;
            _twin = twin;
            _vertex = vertex;
            _edge = edge;
            _face = face;
        }

    private:
        Halfedge(unsigned int id) : _id(id) {
        }
        unsigned int _id = 0;
        HalfedgeRef _twin, _next;
        VertexRef _vertex;
        EdgeRef _edge;
        FaceRef _face;
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
        return halfedges.insert(halfedges.end(), Halfedge(next_id++));
    }
    VertexRef new_vertex() {
        return vertices.insert(vertices.end(), Vertex(next_id++));
    }
    EdgeRef new_edge() {
        return edges.insert(edges.end(), Edge(next_id++));
    }
    FaceRef new_face(bool boundary = false) {
        return faces.insert(faces.end(), Face(next_id++, boundary));
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

    /// Check if half-edge mesh is valid
    std::optional<std::pair<ElementRef, std::string>> validate();
    std::optional<std::pair<ElementRef, std::string>> warnings();

    //////////////////////////////////////////////////////////////////////////////////////////
    // End methods students should use, begin internal methods - you don't need to use these
    //////////////////////////////////////////////////////////////////////////////////////////

    // Various ways of constructing meshes
    Halfedge_Mesh();
    Halfedge_Mesh(const GL::Mesh& mesh);
    Halfedge_Mesh(const std::vector<std::vector<Index>>& polygons, const std::vector<Vec3>& verts);
    Halfedge_Mesh(const Halfedge_Mesh& src) = delete;
    Halfedge_Mesh(Halfedge_Mesh&& src) = default;
    ~Halfedge_Mesh() = default;

    // Various ways of copying meshes
    void operator=(const Halfedge_Mesh& src) = delete;
    Halfedge_Mesh& operator=(Halfedge_Mesh&& src) = default;
    void copy_to(Halfedge_Mesh& mesh);
    ElementRef copy_to(Halfedge_Mesh& mesh, unsigned int eid);

    /// Clear mesh of all elements.
    void clear();
    /// Creates new sub-divided mesh with provided scheme
    bool subdivide(SubD strategy);
    /// Export to renderable vertex-index mesh. Indexes the mesh.
    void to_mesh(GL::Mesh& mesh, bool split_faces) const;
    /// Create mesh from polygon list
    std::string from_poly(const std::vector<std::vector<Index>>& polygons,
                          const std::vector<Vec3>& verts);
    /// Create mesh from renderable triangle mesh (beware of connectivity, does not de-duplicate
    /// vertices)
    std::string from_mesh(const GL::Mesh& mesh);

    /// WARNING: erased elements stay in the element lists until do_erase()
    /// or validate() are called
    void do_erase();

    void mark_dirty();
    bool flipped() const {
        return flip_orientation;
    }
    void flip() {
        flip_orientation = !flip_orientation;
    };
    bool render_dirty_flag = false;

    Vec3 normal_of(ElementRef elem);
    static Vec3 center_of(ElementRef elem);
    static unsigned int id_of(ElementRef elem);

private:
    std::list<Vertex> vertices;
    std::list<Edge> edges;
    std::list<Face> faces;
    std::list<Halfedge> halfedges;

    unsigned int next_id;
    bool flip_orientation = false;

    std::set<VertexRef> verased;
    std::set<EdgeRef> eerased;
    std::set<FaceRef> ferased;
    std::set<HalfedgeRef> herased;
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
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::EdgeRef> {
    uint64_t operator()(Halfedge_Mesh::EdgeRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::FaceRef> {
    uint64_t operator()(Halfedge_Mesh::FaceRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::HalfedgeRef> {
    uint64_t operator()(Halfedge_Mesh::HalfedgeRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::VertexCRef> {
    uint64_t operator()(Halfedge_Mesh::VertexCRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::EdgeCRef> {
    uint64_t operator()(Halfedge_Mesh::EdgeCRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::FaceCRef> {
    uint64_t operator()(Halfedge_Mesh::FaceCRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
template<> struct hash<Halfedge_Mesh::HalfedgeCRef> {
    uint64_t operator()(Halfedge_Mesh::HalfedgeCRef key) const {
        static const std::hash<unsigned int> h;
        return h(key->id());
    }
};
} // namespace std
