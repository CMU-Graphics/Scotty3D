
#include "halfedge.h"
#include "indexed.h"

#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

Halfedge_Mesh Halfedge_Mesh::copy() const {

	std::unordered_map< Vertex const *, VertexRef > vertex_map;
	std::unordered_map< Edge const *, EdgeRef > edge_map;
	std::unordered_map< Face const *, FaceRef > face_map;
	std::unordered_map< Halfedge const *, HalfedgeRef > halfedge_map;

	Halfedge_Mesh mesh;

	//new mesh should also have the same next_id as this one:
	mesh.next_id = next_id;

	// Copy geometry from the original mesh and create a map from
	// pointers in the original mesh to those in the new mesh.
	for (VertexCRef v = vertices_begin(); v != vertices_end(); v++) {
		VertexRef vn = mesh.vertices.insert(mesh.vertices.end(), *v);
		vertex_map[&*v] = vn;
	}
	for (EdgeCRef e = edges_begin(); e != edges_end(); e++) {
		EdgeRef en = mesh.edges.insert(mesh.edges.end(), *e);
		edge_map[&*e] = en;
	}
	for (FaceCRef f = faces_begin(); f != faces_end(); f++) {
		FaceRef fn = mesh.faces.insert(mesh.faces.end(), *f);
		face_map[&*f] = fn;
	}
	for (HalfedgeCRef h = halfedges_begin(); h != halfedges_end(); h++) {
		HalfedgeRef hn = mesh.halfedges.insert(mesh.halfedges.end(), *h);
		halfedge_map[&*h] = hn;
	}

	// "Search and replace" old pointers with new ones.
	for (HalfedgeRef he = mesh.halfedges_begin(); he != mesh.halfedges_end(); he++) {
		he->next = halfedge_map.at(&*he->next);
		he->twin = halfedge_map.at(&*he->twin);
		he->vertex = vertex_map.at(&*he->vertex);
		he->edge = edge_map.at(&*he->edge);
		he->face = face_map.at(&*he->face);
	}
	for (VertexRef v = mesh.vertices_begin(); v != mesh.vertices_end(); v++) {
		v->halfedge = halfedge_map.at(&*v->halfedge);
	}
	for (EdgeRef e = mesh.edges_begin(); e != mesh.edges_end(); e++) {
		e->halfedge = halfedge_map.at(&*e->halfedge);
	}
	for (FaceRef f = mesh.faces_begin(); f != mesh.faces_end(); f++) {
		f->halfedge = halfedge_map.at(&*f->halfedge);
	}

	return mesh;
}

float Halfedge_Mesh::Vertex::angle_defect() const {
	float defect = 2.0f * PI_F;

	auto h = halfedge;
	do {
		auto e0 = (h->twin->vertex->position - position);
		auto e1 = (h->twin->next->twin->vertex->position - position);
		defect -= std::acos(dot(e0.unit(), e1.unit()));
		h = h->twin->next;
	} while (h != halfedge);

	return defect;
}

float Halfedge_Mesh::Vertex::gaussian_curvature() const {
	float defect = angle_defect();
	float area = 0.0f;

	auto h = halfedge;
	do {
		area += h->face->area() / h->face->degree();
		h = h->twin->next;
	} while (h != halfedge);

	return defect / area;
}

Vec3 Halfedge_Mesh::Vertex::neighborhood_center() const {

	Vec3 c;
	float d = 0.0f; // degree (i.e., number of neighbors)

	// Iterate over neighbors.
	HalfedgeCRef h = halfedge;
	do {
		// Add the contribution of the neighbor,
		// and increment the number of neighbors.
		c += h->next->vertex->position;
		d += 1.0f;
		h = h->twin->next;
	} while (h != halfedge);

	c /= d; // compute the average
	return c;
}

uint32_t Halfedge_Mesh::Vertex::degree() const {
	uint32_t d = 0;
	HalfedgeCRef h = halfedge;
	do {
		d++;
		h = h->twin->next;
	} while (h != halfedge);
	return d;
}

float Halfedge_Mesh::Face::area() const {
	float a = 0.0f;
	HalfedgeCRef h = halfedge;
	Vec3 b = h->vertex->position;
	h = h->next;
	do {
		Vec3 pi = h->vertex->position - b;
		Vec3 pj = h->next->vertex->position - b;
		a += 0.5f * cross(pi, pj).norm();
		h = h->next;
	} while (h != halfedge);
	return a;
}

uint32_t Halfedge_Mesh::Face::degree() const {
	uint32_t d = 0;
	HalfedgeCRef h = halfedge;
	do {
		d++;
		h = h->next;
	} while (h != halfedge);
	return d;
}

bool Halfedge_Mesh::Vertex::on_boundary() const {
	bool bound = false;
	HalfedgeCRef h = halfedge;
	do {
		bound = bound || h->is_boundary();
		h = h->twin->next;
	} while (!bound && h != halfedge);
	return bound;
}

bool Halfedge_Mesh::has_boundary() const {
	for (const auto& f : faces) {
		if (f.boundary) return true;
	}
	return false;
}

size_t Halfedge_Mesh::n_boundaries() const {
	size_t n = 0;
	for (const auto& f : faces) {
		if (f.boundary) ++n;
	}
	return n;
}

bool Halfedge_Mesh::Edge::on_boundary() const {
	return halfedge->is_boundary() || halfedge->twin->is_boundary();
}

Vec3 Halfedge_Mesh::Vertex::normal() const {
	Vec3 n = Vec3(0.0f, 0.0f, 0.0f);
	Vec3 pi = position;
	HalfedgeCRef h = halfedge;
	//walk clockwise around the vertex:
	do {
		Vec3 pk = h->next->vertex->position;
		h = h->twin->next;
		Vec3 pj = h->next->vertex->position;
		//pi,pk,pj is a ccw-oriented triangle covering the area of h->face incident on the vertex
		if (!h->face->boundary) n += cross(pj - pi, pk - pi);
	} while (h != halfedge);
	return n.unit();
}

Vec3 Halfedge_Mesh::Edge::normal() const {
	return (halfedge->face->normal() + halfedge->twin->face->normal()).unit();
}

Vec3 Halfedge_Mesh::Face::normal() const {
	Vec3 n;
	HalfedgeCRef h = halfedge;
	do {
		Vec3 pi = h->vertex->position;
		Vec3 pj = h->next->vertex->position;
		n += cross(pi, pj);
		h = h->next;
	} while (h != halfedge);
	return n.unit();
}

float Halfedge_Mesh::Edge::length() const {
	return (halfedge->vertex->position - halfedge->twin->vertex->position).norm();
}

Vec3 Halfedge_Mesh::Edge::center() const {
	return 0.5f * (halfedge->vertex->position + halfedge->twin->vertex->position);
}

Vec3 Halfedge_Mesh::Face::center() const {
	Vec3 c;
	float d = 0.0f;
	HalfedgeCRef h = halfedge;
	do {
		c += h->vertex->position;
		d += 1.0f;
		h = h->next;
	} while (h != halfedge);
	return c / d;
}

uint32_t Halfedge_Mesh::id_of(ElementRef elem) {
	uint32_t id;
	std::visit(overloaded{[&](VertexRef vert) { id = vert->id; },
	                      [&](EdgeRef edge) { id = edge->id; },
	                      [&](FaceRef face) { id = face->id; },
	                      [&](HalfedgeRef halfedge) { id = halfedge->id; }},
	           elem);
	return id;
}

Vec3 Halfedge_Mesh::normal_of(Halfedge_Mesh::ElementRef elem) {
	Vec3 n;
	std::visit(overloaded{[&](VertexRef vert) { n = vert->normal(); },
	                      [&](EdgeRef edge) { n = edge->normal(); },
	                      [&](FaceRef face) { n = face->normal(); },
	                      [&](HalfedgeRef he) { n = he->edge->normal(); }},
	           elem);
	return n;
}

Vec3 Halfedge_Mesh::center_of(Halfedge_Mesh::ElementRef elem) {
	Vec3 pos;
	std::visit(overloaded{[&](VertexRef vert) { pos = vert->center(); },
	                      [&](EdgeRef edge) { pos = edge->center(); },
	                      [&](FaceRef face) { pos = face->center(); },
	                      [&](HalfedgeRef he) { pos = he->edge->center(); }},
	           elem);
	return pos;
}

std::optional<std::pair<Halfedge_Mesh::ElementRef, std::string>> Halfedge_Mesh::validate() {

	for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
		Vec3 p = v->position;
		bool finite = std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
		if (!finite) return {{v, "A vertex position was set to a non-finite value."}};
	}

	std::unordered_map<VertexRef, std::set<HalfedgeRef>> v_accessible;
	std::unordered_map<EdgeRef, std::set<HalfedgeRef>> e_accessible;
	std::unordered_map<FaceRef, std::set<HalfedgeRef>> f_accessible;
	std::set<HalfedgeRef> permutation;

	// Check valid halfedge permutation
	for (HalfedgeRef h = halfedges_begin(); h != halfedges_end(); h++) {

		if (herased.find(h) != herased.end()) continue;

		if (herased.find(h->next) != herased.end()) {
			return {{h, "A live halfedge's next was erased!"}};
		}
		if (herased.find(h->twin) != herased.end()) {
			return {{h, "A live halfedge's twin was erased!"}};
		}
		if (verased.find(h->vertex) != verased.end()) {
			return {{h, "A live halfedge's vertex was erased!"}};
		}
		if (ferased.find(h->face) != ferased.end()) {
			return {{h, "A live halfedge's face was erased!"}};
		}
		if (eerased.find(h->edge) != eerased.end()) {
			return {{h, "A live halfedge's edge was erased!"}};
		}

		// Check whether each halfedge's next points to a unique halfedge
		if (permutation.find(h->next) == permutation.end()) {
			permutation.insert(h->next);
		} else {
			return {{h->next, "A halfedge is the next of multiple halfedges!"}};
		}
	}

	// Check whether each halfedge incident on a vertex points to that vertex
	for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {

		if (verased.find(v) != verased.end()) continue;

		HalfedgeRef h = v->halfedge;
		if (herased.find(h) != herased.end()) {
			return {{v, "A vertex's halfedge is erased!"}};
		}

		std::set<HalfedgeRef> accessible;
		do {
			accessible.insert(h);
			if (h->vertex != v) {
				return {{h, "A vertex's halfedge does not point to that vertex!"}};
			}
			h = h->twin->next;
		} while (h != v->halfedge);
		v_accessible[v] = std::move(accessible);
	}

	// Check whether each halfedge incident on an edge points to that edge
	for (EdgeRef e = edges_begin(); e != edges_end(); e++) {

		if (eerased.find(e) != eerased.end()) continue;

		HalfedgeRef h = e->halfedge;
		if (herased.find(h) != herased.end()) {
			return {{e, "An edge's halfedge is erased!"}};
		}

		std::set<HalfedgeRef> accessible;
		do {
			accessible.insert(h);
			if (h->edge != e) {
				return {{h, "An edge's halfedge does not point to that edge!"}};
			}
			h = h->twin;
		} while (h != e->halfedge);
		e_accessible[e] = std::move(accessible);
	}

	// Check whether each halfedge incident on a face points to that face
	for (FaceRef f = faces_begin(); f != faces_end(); f++) {

		if (ferased.find(f) != ferased.end()) continue;

		HalfedgeRef h = f->halfedge;
		if (herased.find(h) != herased.end()) {
			return {{f, "A face's halfedge is erased!"}};
		}

		std::set<HalfedgeRef> accessible;
		do {
			accessible.insert(h);
			if (h->face != f) {
				return {{h, "A face's halfedge does not point to that face!"}};
			}
			h = h->next;
		} while (h != f->halfedge);
		f_accessible[f] = std::move(accessible);
	}

	for (HalfedgeRef h = halfedges_begin(); h != halfedges_end(); h++) {

		if (herased.find(h) != herased.end()) continue;

		// Check whether each halfedge was pointed to by a halfedge
		if (permutation.find(h) == permutation.end()) {
			return {{h, "A halfedge is the next of zero halfedges!"}};
		}

		// Check twin relationships
		if (h->twin == h) {
			return {{h, "A halfedge's twin is itself!"}};
		}
		if (h->twin->twin != h) {
			return {{h, "A halfedge's twin's twin is not itself!"}};
		}

		// Check that the halfedge can be found via its vertex, edge, and face
		if (v_accessible[h->vertex].count(h) == 0) {
			return {{h, "A halfedge is not accessible from its vertex!"}};
		}
		if (e_accessible[h->edge].count(h) == 0) {
			return {{h, "A halfedge is not accessible from its edge!"}};
		}
		if (f_accessible[h->face].count(h) == 0) {
			return {{h, "A halfedge is not accessible from its face!"}};
		}
	}

	do_erase();
	return std::nullopt;
}

void Halfedge_Mesh::set_corner_normals(float threshold) {
	//first, figure out which edges to consider sharp for this operation:
	std::unordered_set< Edge const * > sharp_edges;
	sharp_edges.reserve(edges.size());

	//all edges between boundary and non-boundary get marked sharp regardless of mode:
	for (auto const &edge : edges) {
		if (edge.halfedge->is_boundary() != edge.halfedge->twin->is_boundary()) {
			sharp_edges.emplace(&edge);
		}
	}

	if (threshold >= 180.0f) {
		//"smooth mode" -- all other edges are considered smooth
	} else {
		//"flat mode" / "auto mode" -- any edges which are marked sharp or have face angle <= threshold get marked sharp:
		float cos_threshold = std::cos( Radians( std::clamp(threshold, 0.0f, 180.0f) ) );
		if (threshold <= 0.0f) cos_threshold = 2.0f; //make sure everything is sharp
		for (auto const &edge : edges) {
			//get adjacent halfedges:
			HalfedgeRef h1 = edge.halfedge;
			HalfedgeRef h2 = h1->twin;
			if (h1->face->boundary || h2->face->boundary) {
				//don't care about edges boundary-boundary, and inside-boundary already marked.
				//thus: nothing to do here
			} else if (edge.sharp) {
				//flagged as sharp, so mark it sharp:
				sharp_edges.emplace(&edge);
			} else {
				//inside-inside edge, non-marked, check angle:
				Vec3 n1 = h1->face->normal();
				Vec3 n2 = h2->face->normal();
				float cos = dot(n1,n2);
				if (cos <= cos_threshold) {
					//treat as sharp:
					sharp_edges.emplace(&edge);
				}
			}
		}
	}

	std::cout << "Marked " << sharp_edges.size() << " of " << n_edges() << " edges as sharp." << std::endl; //DEBUG

	//clear current corner normals:
	for (auto h = halfedges.begin(); h != halfedges.end(); ++h) {
		h->corner_normal = Vec3{0.0f, 0.0f, 0.0f};
	}

	//now circulate all vertices to set normals:
	for (auto const &v : vertices) {
		//get halfedge leaving this vertex:
		HalfedgeRef begin = v.halfedge;
		assert(&*begin->vertex == &v);

		//circulate begin until it is at a sharp edge (thus, the next corner starts a smoothing group):
		do {
			if (sharp_edges.count(&*begin->edge)) break;
			begin = begin->twin->next;
		} while (begin != v.halfedge); //could be all one big happy smoothing group

		//store all corners around the vertex:
		struct Corner {
			HalfedgeRef in; //halfedge pointing to v
			HalfedgeRef out; //halfedge pointing away from v
			Vec3 weighted_normal; //face normal at corner, weighted... somehow (see below)
		};

		std::vector< std::vector< Corner > > groups;
		HalfedgeRef h = begin;
		do {
			//start a new smoothing group on sharp edges (or at the very first edge):
			if (h == begin || sharp_edges.count(&*h->edge)) {
				groups.emplace_back();
			}
			//add corner after h to current smoothing group:
			Corner corner;
			corner.in = h->twin;
			corner.out = h->twin->next;
			assert(corner.in->face == corner.out->face); //PARANOIA
			{ //compute some sort of weighted normal:
				assert(&*corner.in->vertex != &v);
				assert(&*corner.in->twin->vertex == &v);
				Vec3 from = corner.in->vertex->position - v.position;

				assert(&*corner.out->vertex == &v);
				assert(&*corner.out->twin->vertex != &v);
				Vec3 to = corner.out->twin->vertex->position - v.position;
				/*
				//basic area weighting (weird with non-flat faces and reflex vertices):
				corner.weighted_normal = cross(to - v.position, from - v.position);
				*/
				/*//sort sort of angle weighting thing -- this never works as well as one would hope:
				//...still needs work for reflex angles also
				float angle = std::atan2(cross(from,to).norm(), dot(from, to));
				corner.weighted_normal = angle * corner.in->face->normal();
				*/
				//some other sort of slightly fancy area weighting:
				corner.weighted_normal = cross(to - v.position, from - v.position).norm() * corner.in->face->normal();
			}
			groups.back().emplace_back(corner);

			//advance h:
			h = h->twin->next;
		} while (h != begin);

		//compute weighted normals per-corner:
		for (auto const &group : groups) {
			assert(!group.empty());
			if (group[0].in->is_boundary()) {
				//boundary group.
				//PARANOIA:
				for (auto const &corner : group) {
					assert(corner.in->is_boundary());
				}
				//no need for normals on boundary corners
				continue;
			}
			//compute weighted normal:
			Vec3 sum = Vec3{0.0f, 0.0f, 0.0f};
			for (auto const &corner : group) {
				sum += corner.weighted_normal;
			}
			//normalize:
			sum = sum.unit();
			//assign to all corners in group:
			for (auto const &corner : group) {
				assert(&*corner.out->vertex == &v);
				corner.out->corner_normal = sum;
			}
		}
	}

	//normals computed!
}

void Halfedge_Mesh::set_corner_uvs_per_face() {
	//clear existing UVs:
	for (auto &halfedge : halfedges) {
		halfedge.corner_uv = Vec2(0.0f, 0.0f);
	}
	
	//set UVs per-face:
	for (auto const &face : faces) {
		if (face.boundary) continue;

		//come up with a plane perpendicular-ish to the face:
		Vec3 n = face.normal();
		Vec3 p1;
		if (std::abs(n.x) < std::abs(n.y) && std::abs(n.x) < std::abs(n.z)) {
			p1 = Vec3(1.0f, 0.0f, 0.0f);
		} else if (std::abs(n.y) < std::abs(n.z)) {
			p1 = Vec3(0.0f, 1.0f, 0.0f);
		} else {
			p1 = Vec3(0.0f, 0.0f, 1.0f);
		}
		p1 = (p1 - dot(p1, n) * n).unit();
		Vec3 p2 = cross(n, p1);

		//find bounds of face on plane:
		Vec2 min = Vec2(std::numeric_limits< float >::infinity(), std::numeric_limits< float >::infinity());
		Vec2 max = Vec2(-std::numeric_limits< float >::infinity(), -std::numeric_limits< float >::infinity());
		HalfedgeRef v = face.halfedge;
		do {
			Vec2 pt = Vec2(dot(p1, v->vertex->position), dot(p2, v->vertex->position));
			min = hmin(min, pt);
			max = hmax(max, pt);
			v = v->next;
		} while (v != face.halfedge);

		//set corner uvs based on position within bounds:
		do {
			Vec2 pt = Vec2(dot(p1, v->vertex->position), dot(p2, v->vertex->position));
			v->corner_uv = Vec2(
				(pt.x - min.x) / (max.x - min.x),
				(pt.y - min.y) / (max.y - min.y)
			);
			v = v->next;
		} while (v != face.halfedge);
	}
}


void Halfedge_Mesh::set_corner_uvs_project(Vec3 origin, Vec3 u_axis, Vec3 v_axis) {

	u_axis /= u_axis.norm_squared();
	v_axis /= v_axis.norm_squared();

	for (auto &halfedge : halfedges) {
		if (halfedge.is_boundary()) {
			halfedge.corner_uv = Vec2(0.0f, 0.0f);
		} else {
			halfedge.corner_uv = Vec2(
				dot(halfedge.vertex->position - origin, u_axis),
				dot(halfedge.vertex->position - origin, v_axis)
			);
		}
	}
}


void Halfedge_Mesh::do_erase() {
	for (auto& v : verased) {
		vertices.erase(v);
	}
	for (auto& e : eerased) {
		edges.erase(e);
	}
	for (auto& f : ferased) {
		faces.erase(f);
	}
	for (auto& h : herased) {
		halfedges.erase(h);
	}
	verased.clear();
	eerased.clear();
	ferased.clear();
	herased.clear();
}

namespace std {
template< typename A, typename B >
struct hash< std::pair< A, B > > {
	size_t operator()(std::pair< A, B > const &key) const {
		static const std::hash< A > ha;
		static const std::hash< B > hb;
		size_t hf = ha(key.first);
		size_t hs = hb(key.second);
		//NOTE: if this were C++20 we could use std::rotr or std::rotl
		return hf ^ (hs << (sizeof(size_t)*4)) ^ (hs >> (sizeof(size_t)*4));
	}
};
}

Halfedge_Mesh Halfedge_Mesh::from_indexed_faces(std::vector< Vec3 > const &vertices_, std::vector< std::vector< Index > > const &faces_) {

	Halfedge_Mesh mesh;

	std::vector< VertexRef > vertices; //for quick lookup of vertices by index
	vertices.reserve(vertices_.size());
	for (auto const &v : vertices_) {
		vertices.emplace_back(mesh.new_vertex());
		vertices.back()->position = v;
	}


	std::unordered_map< std::pair< Index, Index >, HalfedgeRef > halfedges; //for quick lookup of halfedges by from/to vertex index

	//helper to add a face (and, later, boundary):
	auto add_loop = [&](std::vector< Index > const &loop, bool boundary) {
		assert(loop.size() >= 3);
		
		for (uint32_t j = 0; j < loop.size(); ++j) {
			//omit adding a face with vertices of the same index, otherwise crashes on cylinder/cone
			if (loop[j] == loop[(j + 1) % loop.size()]) { return; }
		}

		FaceRef face = mesh.new_face(boundary);
		HalfedgeRef prev = mesh.halfedges_end(); //keep track of previous edge around face to set next pointer
		for (uint32_t i = 0; i < loop.size(); ++i) {
			Index a = loop[i];
			Index b = loop[(i + 1) % loop.size()];
			assert(a != b);

			HalfedgeRef halfedge = mesh.new_halfedge();
			if (i == 0) face->halfedge = halfedge; //store first edge as face's halfedge pointer
			halfedge->vertex = vertices[a];

			//if first to mention vertex, set vertex's halfedge pointer:
			if (vertices[a]->halfedge == mesh.halfedges_end()) {
				assert(!boundary); //boundary faces should never be mentioning novel vertices, since they are created second
				vertices[a]->halfedge = halfedge;
			}
			halfedge->face = face;

			auto inserted = halfedges.emplace(std::make_pair(a,b), halfedge);
			assert(inserted.second); //if edge mentioned more than once in the same direction, not an oriented, manifold mesh

			auto twin = halfedges.find(std::make_pair(b,a));
			if (twin == halfedges.end()) {
				assert(!boundary); //boundary faces exist only to complete edges so should *always* match
				//not twinned yet -- create an edge just for this halfedge:
				EdgeRef edge = mesh.new_edge();
				halfedge->edge = edge;
				edge->halfedge = halfedge;
			} else {
				//found a twin -- connect twin pointers and reference its edge:
				assert(twin->second->twin == mesh.halfedges_end());
				halfedge->twin = twin->second;
				halfedge->edge = twin->second->edge;
				twin->second->twin = halfedge;
			}

			if (i != 0) prev->next = halfedge; //set previous halfedge's next pointer
			prev = halfedge;
		}
		prev->next = face->halfedge; //set next pointer for last halfedge to first edge
	};

	//add all faces:
	for (auto const &loop : faces_) {
		add_loop(loop, false);
	}

	// All halfedges created so far have valid next pointers, but some may be missing twins because they are at a boundary.
	
	std::map< Index, Index > next_on_boundary;

	//first, look for all un-twinned halfedges to figure out the shape of the boundary:
	for (auto const &[ from_to, halfedge ] : halfedges) {
		if (halfedge->twin == mesh.halfedges_end()) {
			auto ret = next_on_boundary.emplace(from_to.second, from_to.first); //twin needed on the boundary
			assert(ret.second); //every boundary vertex should have a unique successor because the boundary is "half-disc-like"
		}
	}

	//now pull out boundary loops until all edges are exhausted:
	while (!next_on_boundary.empty()) {
		std::vector< Index > loop;
		loop.emplace_back(next_on_boundary.begin()->first);

		do {
			//look up next hop on the boundary:
			auto next = next_on_boundary.find(loop.back());
			//should never be dead ends on boundary:
			assert(next != next_on_boundary.end());

			//add next hop to loop:
			loop.emplace_back(next->second);
			//...and remove from nexts structure:
			next_on_boundary.erase(next);
		} while (loop[0] != loop.back());

		loop.pop_back(); //remove duplicated first/last element

		assert(loop.size() >= 3); //all faces must be non-degenerate

		//add boundary loop:
		add_loop(loop, true);
	}

	//with boundary faces created, mesh should be ready to go with all edges nicely twinned.

	//PARANOIA: this should never happen:
	auto error = mesh.validate();
	if (error) {
		std::cerr << "Halfedge_Mesh from_indexed_faces failed validation: " << error->second << std::endl;
		assert(0);
	}

	return mesh;
}

Halfedge_Mesh Halfedge_Mesh::from_indexed_mesh(Indexed_Mesh const &indexed_mesh) {

	//extract vertex positions and face indices from indexed_mesh:
	std::vector< Vec3 > indexed_vertices;
	indexed_vertices.reserve(indexed_mesh.vertices().size());
	for (auto const &v : indexed_mesh.vertices()) {
		indexed_vertices.emplace_back(v.pos);
	}

	std::vector< std::vector< Index > > indexed_faces;
	assert(indexed_mesh.indices().size() % 3 == 0);
	indexed_faces.reserve(indexed_mesh.indices().size() / 3);
	for (uint32_t i = 0; i < indexed_mesh.indices().size(); i += 3) {
		indexed_faces.emplace_back(indexed_mesh.indices().begin() + i, indexed_mesh.indices().begin() + i + 3);
	}

	//build halfedge mesh with the extracted vertex/face data:
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces(indexed_vertices, indexed_faces);

	//now copy corner data:
	auto indexed_vertex = indexed_mesh.vertices().begin();
	for (auto vertex = mesh.vertices_begin(); vertex != mesh.vertices_end(); ++vertex) {
		assert(indexed_vertex != indexed_mesh.vertices().end());

		HalfedgeRef h = vertex->halfedge;
		do {
			if (!h->face->boundary) {
				h->corner_normal = indexed_vertex->norm;
				h->corner_uv = indexed_vertex->uv;
			}
			h = h->twin->next;
		} while (h != vertex->halfedge);
		//copy data from indexed_vertex -> all (non-boundary) halfedges in vertex->halfedge star.

		++indexed_vertex;
	}
	assert(indexed_vertex == indexed_mesh.vertices().end());

	return mesh;
}

Halfedge_Mesh Halfedge_Mesh::cube(float r) {
	Halfedge_Mesh mesh = Halfedge_Mesh::from_indexed_faces(
		std::vector< Vec3 >{
			Vec3{-r, -r, -r}, Vec3{ r, -r, -r},
			Vec3{-r,  r, -r}, Vec3{ r,  r, -r},
			Vec3{-r, -r,  r}, Vec3{ r, -r,  r},
			Vec3{-r,  r,  r}, Vec3{ r,  r,  r}
		},
		std::vector< std::vector< Index > >{
			{0,2,3,1}, {4,5,7,6},
			{0,1,5,4}, {1,3,7,5},
			{3,2,6,7}, {2,0,4,6},
		}
	);

	for (auto &edge : mesh.edges) {
		edge.sharp = true;
	}
	mesh.set_corner_normals(0.0f);
	mesh.set_corner_uvs_per_face();

	return mesh;
}

float Halfedge_Mesh::radius() const {
	float max_radius = 0.0f;
	for (auto vertex = vertices_begin(); vertex != vertices_end(); ++vertex) {
		max_radius = std::max(max_radius, vertex->position.norm());
	}
	return max_radius;
}

bool Halfedge_Mesh::subdivide(SubD strategy) {

	warn("TODO: subdivide needs re-write to work with new from_indexed_faces.");
	return false;

/* TODO: need to figure out how to pass around corner data properly.

	switch (strategy) {
	case SubD::linear: {
		linear_subdivide_positions();
	} break;

	case SubD::catmull_clark: {
		if (has_boundary()) return false; //TODO: just handle boundary cases below.
		catmullclark_subdivide_positions();
	} break;

	case SubD::loop: {
		if (has_boundary()) return false; //TODO: just handle boundary cases!
		for (FaceRef f = faces_begin(); f != faces_end(); f++) {
			if (f->degree() != 3) return false;
		}
		loop_subdivide();
		return true;
	} break;

	default: assert(false);
	}

	std::vector<std::vector<Index>> polys;
	std::vector<Indexed_Mesh::Vert> verts;
	std::unordered_map<uint32_t, Index> layout;

	Index idx = 0;
	size_t nV = vertices.size();
	size_t nE = edges.size();
	size_t nF = faces.size() - n_boundaries();
	verts.reserve(nV + nE + nF);

	for (auto v : vertices) {
		verts.emplace_back(
	}

	for (VertexRef v = vertices_begin(); v != vertices_end(); v++, idx++) {
		verts[idx].position = v->new_pos;
		layout[v->id] = idx;
	}
	for (EdgeRef e = edges_begin(); e != edges_end(); e++, idx++) {
		verts[idx].pos = e->new_pos;
		verts[idx].uv = 0.5f * (e->halfedge->vertex->uv + e->halfedge->twin->vertex->uv);
		layout[e->id] = idx;
	}
	for (FaceRef f = faces_begin(); f != faces_end(); f++) {
		if (f->is_boundary()) continue;
		verts[idx].pos = f->new_pos;
		Vec2 uv;
		uint32_t n = 0;
		HalfedgeRef h = f->halfedge;
		do {
			n++;
			uv += h->vertex->uv;
			h = h->next;
		} while (h != f->halfedge);
		verts[idx].uv = uv / static_cast<float>(n);
		layout[f->id] = idx;
		idx++;
	}

	for (auto f = faces_begin(); f != faces_end(); f++) {
		if (f->is_boundary()) continue;
		Index i = layout[f->id];
		HalfedgeRef h = f->halfedge;
		do {
			Index j = layout[h->edge->id];
			Index k = layout[h->next->vertex->id];
			Index l = layout[h->next->edge->id];
			std::vector<Index> quad = {i, j, k, l};
			polys.push_back(quad);
			h = h->next;
		} while (h != f->halfedge);
	}

	from_poly(polys, verts);
	return true;
	*/
}

void Halfedge_Mesh::flip_orientation() {
	std::unordered_map<uint32_t, VertexRef> he_to_v;
	std::unordered_map<uint32_t, HalfedgeRef> v_to_he;
	for (auto &he : halfedges) {
		he_to_v[he.id] = he.twin->vertex;
	}
	for (auto &v : vertices) {
		v_to_he[v.id] = v.halfedge->twin;
	}
	for (auto &face : faces) {
		//read off halfedges around face:
		std::vector<HalfedgeRef> hs;
		HalfedgeRef h = face.halfedge;
		do {
			hs.emplace_back(h);
			h = h->next;
		} while (h != face.halfedge);

		//reverse face ordering:
		for (uint32_t i = 0; i < hs.size(); ++i) {
			hs[(i+1)%hs.size()]->next = hs[i];
		}
	}
	for (auto &he : halfedges) {
		he.vertex = he_to_v[he.id];
	}
	for (auto &v : vertices) {
		v.halfedge = v_to_he[v.id];
	}
}
