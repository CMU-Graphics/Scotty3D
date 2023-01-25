
#include "halfedge.h"
#include "indexed.h"

#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

void Halfedge_Mesh::interpolate_data(std::vector< VertexCRef > const &from, VertexRef to) {
	assert(from.size() >= 1);

	std::map< uint32_t, float > weights;
	for (VertexCRef const v : from) {
		for (Vertex::Bone_Weight const &bw : v->bone_weights) {
			weights.emplace(bw.bone, 0.0f).first->second += bw.weight;
		}
	}

	to->bone_weights.clear();
	to->bone_weights.reserve(weights.size());
	float div = 1.0f / float(from.size());
	for (auto const &bw : weights) {
		to->bone_weights.emplace_back(Vertex::Bone_Weight{ bw.first, div * bw.second });
	}
}

void Halfedge_Mesh::interpolate_data(std::vector< HalfedgeCRef > const &from, HalfedgeRef to) {
	assert(from.size() >= 1);

	Vec2 uv_sum = Vec2(0.0f, 0.0f);
	Vec3 normal_sum = Vec3(0.0f, 0.0f, 0.0f);

	for (HalfedgeCRef const h : from) {
		uv_sum += h->corner_uv;
		normal_sum += h->corner_normal;
	}
	to->corner_uv = uv_sum / float(from.size());
	if (normal_sum == Vec3(0.0f, 0.0f, 0.0f)) {
		to->corner_normal = Vec3(0.0f, 0.0f, 0.0f);
	} else {
		to->corner_normal = normal_sum.unit();
	}
}

Halfedge_Mesh::VertexRef Halfedge_Mesh::emplace_vertex() {
	VertexRef vertex;
	if (free_vertices.empty()) {
		//allocate a new vertex:
		vertex = vertices.emplace(vertices.end(), Vertex(next_id++));
	} else {
		//recycle vertex from free list:
		vertex = free_vertices.begin();
		vertices.splice(vertices.end(), free_vertices, free_vertices.begin());
		*vertex = Vertex(next_id++); //set to default values
	}
	//make sure vertex doesn't reference anything:
	vertex->halfedge = halfedges.end();
	return vertex;
}

Halfedge_Mesh::EdgeRef Halfedge_Mesh::emplace_edge(bool sharp) {
	EdgeRef edge;
	if (free_edges.empty()) {
		//allocate a new edge:
		edge = edges.emplace(edges.end(), Edge(next_id++, sharp));
	} else {
		//recycle edge from free list:
		edge = free_edges.begin();
		edges.splice(edges.end(), free_edges, free_edges.begin());
		*edge = Edge(next_id++, sharp); //set to default values
	}
	//make sure edge doesn't reference anything:
	edge->halfedge = halfedges.end();
	return edge;
}

Halfedge_Mesh::FaceRef Halfedge_Mesh::emplace_face(bool boundary) {
	FaceRef face;
	if (free_faces.empty()) {
		//allocate a new face:
		face = faces.emplace(faces.end(), Face(next_id++, boundary));
	} else {
		//recycle face from free list:
		face = free_faces.begin();
		faces.splice(faces.end(), free_faces, free_faces.begin());
		*face = Face(next_id++, boundary); //set to default values
	}
	face->halfedge = halfedges.end();
	return face;
}

Halfedge_Mesh::HalfedgeRef Halfedge_Mesh::emplace_halfedge() {
	HalfedgeRef halfedge;
	if (!free_halfedges.empty()) {
		//move from free list:
		halfedge = free_halfedges.begin();
		halfedges.splice(halfedges.end(), free_halfedges, free_halfedges.begin());
		*halfedge = Halfedge(next_id++); //set to default values
	} else {
		//allocate a new halfedge:
		halfedge = halfedges.insert(halfedges.end(), Halfedge(next_id++));
	}
	//set pointers to default values:
	halfedge->twin = halfedges.end();
	halfedge->next = halfedges.end();
	halfedge->vertex = vertices.end();
	halfedge->edge = edges.end();
	halfedge->face = faces.end();
	return halfedge;
}


void Halfedge_Mesh::erase_vertex(VertexRef v) {
	//clear data:
	v->id |= 0x80000000u;
	v->position = Vec3( std::numeric_limits< float >::quiet_NaN(), std::numeric_limits< float >::quiet_NaN(), std::numeric_limits< float >::quiet_NaN() );
	v->bone_weights.clear();

	//clear connectivity:
	v->halfedge = halfedges.end();

	//move to free list:
	free_vertices.splice(free_vertices.end(), vertices, v);
}


void Halfedge_Mesh::erase_edge(EdgeRef e) {
	//clear data:
	e->id |= 0x80000000u;
	e->sharp = false;

	//clear connectivity:
	e->halfedge = halfedges.end();

	//move to free list:
	free_edges.splice(free_edges.end(), edges, e);
}

void Halfedge_Mesh::erase_face(FaceRef f) {
	//clear data:
	f->id |= 0x80000000u;
	f->boundary = false;

	//clear connectivity:
	f->halfedge = halfedges.end();

	//move to free list:
	free_faces.splice(free_faces.end(), faces, f);
}

void Halfedge_Mesh::erase_halfedge(HalfedgeRef h) {
	//clear data:
	h->id |= 0x80000000u;
	h->corner_uv = Vec2(std::numeric_limits< float >::quiet_NaN(), std::numeric_limits< float >::quiet_NaN());
	h->corner_normal = Vec3(std::numeric_limits< float >::quiet_NaN(), std::numeric_limits< float >::quiet_NaN(), std::numeric_limits< float >::quiet_NaN());

	//clear connectivity:
	h->twin = halfedges.end();
	h->next = halfedges.end();
	h->vertex = vertices.end();
	h->edge = edges.end();
	h->face = faces.end();

	//move to free list:
	free_halfedges.splice(free_halfedges.end(), halfedges, h);
}



bool Halfedge_Mesh::Vertex::on_boundary() const {
	bool bound = false;
	HalfedgeCRef h = halfedge;
	do {
		bound = bound || h->face->boundary;
		h = h->twin->next;
	} while (!bound && h != halfedge);
	return bound;
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

uint32_t Halfedge_Mesh::Vertex::degree() const {
	uint32_t d = 0;
	HalfedgeCRef h = halfedge;
	do {
		d++;
		h = h->twin->next;
	} while (h != halfedge);
	return d;
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

bool Halfedge_Mesh::Edge::on_boundary() const {
	return halfedge->face->boundary || halfedge->twin->face->boundary;
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


uint32_t Halfedge_Mesh::id_of(ElementCRef elem) {
	return std::visit( [&](auto ref) -> uint32_t { return ref->id; }, elem );
}

Vec3 Halfedge_Mesh::normal_of(Halfedge_Mesh::ElementCRef elem) {
	Vec3 n;
	std::visit(overloaded{[&](VertexCRef vert) { n = vert->normal(); },
	                      [&](EdgeCRef edge) { n = edge->normal(); },
	                      [&](FaceCRef face) { n = face->normal(); },
	                      [&](HalfedgeCRef he) { n = he->edge->normal(); }},
	           elem);
	return n;
}

Vec3 Halfedge_Mesh::center_of(Halfedge_Mesh::ElementCRef elem) {
	Vec3 pos;
	std::visit(overloaded{[&](VertexCRef vert) { pos = vert->position; },
	                      [&](EdgeCRef edge) { pos = edge->center(); },
	                      [&](FaceCRef face) { pos = face->center(); },
	                      [&](HalfedgeCRef he) { pos = he->edge->center(); }},
	           elem);
	return pos;
}


uint32_t Halfedge_Mesh::id_of(ElementRef elem) { return id_of(const_from(elem)); }
Vec3 Halfedge_Mesh::center_of(ElementRef elem) { return center_of(const_from(elem)); }
Vec3 Halfedge_Mesh::normal_of(ElementRef elem) { return normal_of(const_from(elem)); }

Halfedge_Mesh::ElementCRef Halfedge_Mesh::const_from(ElementRef elem) {
	return std::visit( [&](auto r) { return ElementCRef(r); }, elem );
}


//count boundaries:
size_t Halfedge_Mesh::n_boundaries() const {
	size_t n = 0;
	for (const auto& f : faces) {
		if (f.boundary) ++n;
	}
	return n;
}


//helper used to get set of pointers into a list (used by validate and describe):
template< typename T >
static std::unordered_set< T const * > element_addresses(std::list< T > const &list) {
	std::unordered_set< T const * > address_set;
	for (auto const &e : list) {
		auto ret = address_set.emplace(&e);
		assert(ret.second);
	}
	return address_set;
}

//description of the mesh suitable for debugging:
std::string Halfedge_Mesh::describe() const {

	//for checking whether a reference is held in a given list:
	std::unordered_set< Vertex const * > in_vertices = element_addresses(vertices);
	std::unordered_set< Edge const * > in_edges = element_addresses(edges);
	std::unordered_set< Face const * > in_faces = element_addresses(faces);
	std::unordered_set< Halfedge const * > in_halfedges = element_addresses(halfedges);
	std::unordered_set< Vertex const * > in_free_vertices = element_addresses(free_vertices);
	std::unordered_set< Edge const * > in_free_edges = element_addresses(free_edges);
	std::unordered_set< Face const * > in_free_faces = element_addresses(free_faces);
	std::unordered_set< Halfedge const * > in_free_halfedges = element_addresses(free_halfedges);

	//helpers for describing things:
	auto desc_halfedge = [&,this](HalfedgeRef h) -> std::string {
		if (in_halfedges.count(&*h)) return "h" + std::to_string(h->id);
		else if (in_free_halfedges.count(&*h)) return "h[freed" + std::to_string(h->id & ~0x80000000u) + "]";
		else if (h == halfedges.end()) return "hx";
		else return "h?";
	};
	auto desc_vertex = [&,this](VertexRef v) -> std::string {
		if (in_vertices.count(&*v)) return "v" + std::to_string(v->id);
		else if (in_free_vertices.count(&*v)) return "v[freed" + std::to_string(v->id & ~0x80000000u) + "]";
		else if (v == vertices.end()) return "vx";
		else return "v?";
	};
	auto desc_edge = [&,this](EdgeRef e) -> std::string {
		if (in_edges.count(&*e)) return "e" + std::to_string(e->id);
		else if (in_free_edges.count(&*e)) return "e[freed" + std::to_string(e->id & ~0x80000000u) + "]";
		else if (e == edges.end()) return "ex";
		else return "e?";
	};
	auto desc_face = [&,this](FaceRef f) -> std::string {
		if (in_faces.count(&*f)) return "f" + std::to_string(f->id);
		else if (in_free_faces.count(&*f)) return "f[freed" + std::to_string(f->id & ~0x80000000u) + "]";
		else if (f == faces.end()) return "fx";
		else return "f?";
	};


	std::ostringstream oss;
	oss << "Mesh with " << halfedges.size() << " halfedges, " << vertices.size() << " vertices, " << edges.size() << " edges, and " << faces.size() << " faces:\n";
	for (auto const &h : halfedges) {
		oss << "  [h" << h.id << "] t:" << desc_halfedge(h.twin)
		    << " n:" << desc_halfedge(h.next)
		    << " " << desc_vertex(h.vertex)
		    << " " << desc_edge(h.edge)
		    << " " << desc_face(h.face) << "\n";
	}
	for (auto const &v : vertices) {
		oss << "  [v" << v.id << "] " << desc_halfedge(v.halfedge) << " @ " << v.position << "\n";
	}
	for (auto const &e : edges) {
		oss << "  [e" << e.id << "] " << desc_halfedge(e.halfedge) << "\n";
	}
	for (auto const &f : faces) {
		oss << "  [f" << f.id << "] " << desc_halfedge(f.halfedge) << "\n";
	}

	return oss.str();
}

//helper used to format addresses as strings for display:
template< typename T >
static std::string to_string(T const *addr) {
	std::ostringstream oss;
	oss << reinterpret_cast< void const * >(addr);
	return oss.str();
}

std::optional<std::pair<Halfedge_Mesh::ElementCRef, std::string>> Halfedge_Mesh::validate() const {

	//helpers for error messages:
	auto describe_vertex   = [](VertexCRef v)   { return "Vertex with id " + std::to_string(v->id); };
	auto describe_edge     = [](EdgeCRef e)     { return "Edge with id " + std::to_string(e->id); };
	auto describe_face     = [](FaceCRef f)     { return "Face with id " + std::to_string(f->id); };
	auto describe_halfedge = [](HalfedgeCRef h) { return "Halfedge with id " + std::to_string(h->id); };


	//-----------------------------
	//check element data:
	for (VertexCRef v = vertices.begin(); v != vertices.end(); ++v) {
		if (!std::isfinite(v->position.x)) return {{v, describe_vertex(v) + " has position.x set to a non-finite value: " + std::to_string(v->position.x) + "."}};
		if (!std::isfinite(v->position.y)) return {{v, describe_vertex(v) + " has position.y set to a non-finite value: " + std::to_string(v->position.y) + "."}};
		if (!std::isfinite(v->position.z)) return {{v, describe_vertex(v) + " has position.z set to a non-finite value: " + std::to_string(v->position.z) + "."}};
		for (uint32_t b = 0; b < v->bone_weights.size(); ++b) {
			if (!std::isfinite(v->bone_weights[b].weight)) return {{v, describe_vertex(v) + " has bone_weights[" + std::to_string(b) + "].weight set to a non-finite value: " + std::to_string(v->bone_weights[b].weight) + "."}};
		}
	}

	for (EdgeCRef e = edges.begin(); e != edges.end(); ++e) {
		//NOTE: no edge data to check (both true and false are valid for sharp)
	}

	for (FaceCRef f = faces.begin(); f != faces.end(); ++f) {
		//NOTE: no edge data to check (both true and false are valid for boundary)
	}

	for (HalfedgeCRef h = halfedges.begin(); h != halfedges.end(); ++h) {
		if (!std::isfinite(h->corner_uv.x)) return {{h, describe_halfedge(h) + " has corner_uv.x set to a non-finite value: " + std::to_string(h->corner_uv.x) + "."}};
		if (!std::isfinite(h->corner_uv.y)) return {{h, describe_halfedge(h) + " has corner_uv.y set to a non-finite value: " + std::to_string(h->corner_uv.y) + "."}};
		if (!std::isfinite(h->corner_normal.x)) return {{h, describe_halfedge(h) + " has corner_normal.x set to a non-finite value: " + std::to_string(h->corner_normal.x) + "."}};
		if (!std::isfinite(h->corner_normal.y)) return {{h, describe_halfedge(h) + " has corner_normal.y set to a non-finite value: " + std::to_string(h->corner_normal.y) + "."}};
		if (!std::isfinite(h->corner_normal.z)) return {{h, describe_halfedge(h) + " has corner_normal.z set to a non-finite value: " + std::to_string(h->corner_normal.z) + "."}};
	}

	//-----------------------------
	//all references held by elements are to members of the vertices, edges, faces, or halfedges lists

	//for checking whether a reference is held in a given list:
	std::unordered_set< Vertex const * > in_vertices = element_addresses(vertices);
	std::unordered_set< Edge const * > in_edges = element_addresses(edges);
	std::unordered_set< Face const * > in_faces = element_addresses(faces);
	std::unordered_set< Halfedge const * > in_halfedges = element_addresses(halfedges);

	//helpers for describing things that aren't in the element lists:
	std::unordered_set< Vertex const * > in_free_vertices = element_addresses(free_vertices);
	std::unordered_set< Edge const * > in_free_edges = element_addresses(free_edges);
	std::unordered_set< Face const * > in_free_faces = element_addresses(free_faces);
	std::unordered_set< Halfedge const * > in_free_halfedges = element_addresses(free_halfedges);

	auto describe_missing_vertex = [this,&in_free_vertices](VertexCRef v) -> std::string {
		if (v == vertices.end()) return "past-the-end vertex";
		else if (in_free_vertices.count(&*v)) return "erased vertex with old id " + std::to_string(v->id & 0x7fffffffu);
		else return "out-of-mesh vertex with address " + to_string(&*v);
	};

	auto describe_missing_edge = [this,&in_free_edges](EdgeCRef e) -> std::string {
		if (e == edges.end()) return "past-the-end edge";
		else if (in_free_edges.count(&*e)) return "erased edge with old id " + std::to_string(e->id & 0x7fffffffu);
		else return "out-of-mesh edge with address " + to_string(&*e);
	};

	auto describe_missing_face = [this,&in_free_faces](FaceCRef f) -> std::string {
		if (f == faces.end()) return "past-the-end face";
		else if (in_free_faces.count(&*f)) return "erased face with old id " + std::to_string(f->id & 0x7fffffffu);
		else return "out-of-mesh face with address " + to_string(&*f);
	};

	auto describe_missing_halfedge = [this,&in_free_halfedges](HalfedgeCRef h) -> std::string {
		if (h == halfedges.end()) return "past-the-end halfedge";
		else if (in_free_halfedges.count(&*h)) return "erased halfedge with old id " + std::to_string(h->id & 0x7fffffffu);
		else return "out-of-mesh halfedge with address " + to_string(&*h);
	};

	//check references made by vertices:
	for (VertexCRef v = vertices.begin(); v != vertices.end(); ++v) {
		if (!in_halfedges.count(&*v->halfedge)) return {{v, describe_vertex(v) + " references " + describe_missing_halfedge(v->halfedge) + "."}};
	}

	//check references made by edges:
	for (EdgeCRef e = edges.begin(); e != edges.end(); ++e) {
		if (!in_halfedges.count(&*e->halfedge)) return {{e, describe_edge(e) + " references " + describe_missing_halfedge(e->halfedge) + "."}};
	}

	//check references made by faces:
	for (FaceCRef f = faces.begin(); f != faces.end(); ++f) {
		if (!in_halfedges.count(&*f->halfedge)) return {{f, describe_face(f) + " references " + describe_missing_halfedge(f->halfedge) + "."}};
	}

	//check references made by halfedges:
	for (HalfedgeCRef h = halfedges.begin(); h != halfedges.end(); ++h) {
		if (!in_halfedges.count(&*h->twin)) return {{h, describe_halfedge(h) + " has twin which references " + describe_missing_halfedge(h->twin) + "."}};
		if (!in_halfedges.count(&*h->next)) return {{h, describe_halfedge(h) + " has next which references " + describe_missing_halfedge(h->next) + "."}};
		if (!in_vertices.count(&*h->vertex)) return {{h, describe_halfedge(h) + " references " + describe_missing_vertex(h->vertex) + "."}};
		if (!in_edges.count(&*h->edge)) return {{h, describe_halfedge(h) + " references " + describe_missing_edge(h->edge) + "."}};
		if (!in_faces.count(&*h->face)) return {{h, describe_halfedge(h) + " references " + describe_missing_face(h->face) + "."}};
	}

	//------------------------------
	// - `edge->halfedge(->twin)^n` is a cycle of two halfedges
	//   - this is also exactly the set of halfedges that reference `edge`
	// - `face->halfedge(->next)^n` is a cycle of at least three halfedges
	//   - this is also exactly the set of halfedges that reference `face`
	// - `vertex->halfedge(->twin->next)^n` is a cycle of at least two halfedges
	//   - this is also exactly the set of halfedges that reference `vertex`

	//first, build list of all halfedges that reference every other feature:
	std::unordered_map< VertexCRef, std::unordered_set< HalfedgeCRef > > vertex_halfedges;
	std::unordered_map< EdgeCRef, std::unordered_set< HalfedgeCRef > > edge_halfedges;
	std::unordered_map< FaceCRef, std::unordered_set< HalfedgeCRef > > face_halfedges;
	for (HalfedgeCRef h = halfedges.begin(); h != halfedges.end(); ++h) {
		auto vret = vertex_halfedges[h->vertex].emplace(h);
		assert(vret.second); //every halfedge is unique, so emplace can't fail
		auto eret = edge_halfedges[h->edge].emplace(h);
		assert(eret.second); //every halfedge is unique, so emplace can't fail
		auto fret = face_halfedges[h->face].emplace(h);
		assert(fret.second); //every halfedge is unique, so emplace can't fail
	}

	//check edge->halfedge(->twin)^n:
	for (EdgeCRef e = edges.begin(); e != edges.end(); ++e) {
		std::unordered_set< HalfedgeCRef > to_visit = edge_halfedges[e];
		std::string path = "halfedge";
		HalfedgeCRef h = e->halfedge;
		do {
			if (h->edge != e) return {{e, describe_edge(e) + " has " + path + " of " + describe_halfedge(h) + ", which does not reference the edge."}};
			auto found = to_visit.find(h);
			if (found == to_visit.end()) return {{e, describe_edge(e) + " has halfedge(->twin)^n which is not a cycle."}};
			to_visit.erase(found);

			h = h->twin;
			path += "->twin";
		} while (h != e->halfedge);
		if (!to_visit.empty()) return {{e, describe_edge(e) + " is referenced by " + describe_halfedge(*to_visit.begin()) + ", which is not in halfedge(->twin)^n."}};
		if (edge_halfedges[e].size() != 2) return {{e, describe_edge(e) + " has " + std::to_string(edge_halfedges[e].size()) + " (!= 2) elements in its halfedge(->twin)^n cycle."}};
	}

	//check face->halfedge(->next)^n:
	for (FaceCRef f = faces.begin(); f != faces.end(); ++f) {
		std::unordered_set< HalfedgeCRef > to_visit = face_halfedges[f];
		std::string path = "halfedge";
		HalfedgeCRef h = f->halfedge;
		do {
			if (h->face != f) return {{f, describe_face(f) + " has " + path + " of " + describe_halfedge(h) + ", which does not reference the face."}};
			auto found = to_visit.find(h);
			if (found == to_visit.end()) return {{f, describe_face(f) + " has halfedge(->next)^n which is not a cycle."}};
			to_visit.erase(found);

			h = h->next;
			path += "->next";
		} while (h != f->halfedge);
		if (!to_visit.empty()) return {{f, describe_face(f) + " is referenced by " + describe_halfedge(*to_visit.begin()) + ", which is not in halfedge(->next)^n."}};
		if (face_halfedges[f].size() < 3) return {{f, describe_face(f) + " has " + std::to_string(face_halfedges[f].size()) + " (< 3) elements in its halfedge(->next)^n cycle."}};
	}

	//check vertex->halfedge(->twin->next)^n:
	for (VertexCRef v = vertices.begin(); v != vertices.end(); ++v) {
		std::unordered_set< HalfedgeCRef > to_visit = vertex_halfedges[v];
		std::string path = "halfedge";
		HalfedgeCRef h = v->halfedge;
		do {
			if (h->vertex != v) return {{v, describe_vertex(v) + " has " + path + " of " + describe_halfedge(h) + ", which does not reference the vertex."}};
			auto found = to_visit.find(h);
			if (found == to_visit.end()) return {{v, describe_vertex(v) + " has halfedge(->twin->next)^n which is not a cycle."}};
			to_visit.erase(found);

			h = h->twin->next;
			path += "->twin->next";
		} while (h != v->halfedge);
		if (!to_visit.empty()) return {{v, describe_vertex(v) + " is referenced by " + describe_halfedge(*to_visit.begin()) + ", which is not in halfedge(->twin->next)^n."}};
		if (vertex_halfedges[v].size() < 2) return {{v, describe_vertex(v) + " has " + std::to_string(vertex_halfedges[v].size()) + " (< 2) elements in its halfedge(->twin->next)^n cycle."}};
	}

	//------------------------------
	// - vertices are not orphaned (they have at least one non-boundary face adjacent)
	// - vertices are on at most one boundary face

	for (VertexCRef v = vertices.begin(); v != vertices.end(); ++v) {
		uint32_t non_boundary = 0;
		uint32_t boundary = 0;
		HalfedgeCRef h = v->halfedge;
		do {
			if (h->face->boundary) ++boundary;
			else ++non_boundary;

			h = h->twin->next;
		} while (h != v->halfedge);

		if (non_boundary == 0) return {{v, describe_vertex(v) + " is orphaned (has no adjacent non-boundary faces)."}};
		if (boundary > 1) return {{v, describe_vertex(v) + " is on " + std::to_string(boundary) + " (> 1) boundary faces."}};
	}

	//------------------------------
	// - edges are not orphaned (they have at least one non-boundary face adjacent)
	for (EdgeCRef e = edges.begin(); e != edges.end(); ++e) {
		if (e->halfedge->face->boundary && e->halfedge->twin->face->boundary) return {{e, describe_edge(e) + " is orphaned (has no adjacent non-boundary face)."}};
	}


	//------------------------------
	// - faces are simple (touch each vertex / edge at most once)
	for (FaceCRef f = faces.begin(); f != faces.end(); ++f) {
		std::unordered_set< VertexCRef > touched_vertices;
		std::unordered_set< EdgeCRef > touched_edges;

		HalfedgeCRef h = f->halfedge;
		do {
			if (!touched_vertices.emplace(h->vertex).second) return {{f, describe_face(f) + " touches " + describe_vertex(h->vertex) + " more than once."}};
			if (!touched_edges.emplace(h->edge).second) return {{f, describe_face(f) + " touches " + describe_edge(h->edge) + " more than once."}};

			h = h->next;
		} while (h != f->halfedge);
	}

	return std::nullopt;
}



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
	for (VertexCRef v = vertices.begin(); v != vertices.end(); v++) {
		VertexRef vn = mesh.vertices.insert(mesh.vertices.end(), *v);
		vertex_map[&*v] = vn;
	}
	for (EdgeCRef e = edges.begin(); e != edges.end(); e++) {
		EdgeRef en = mesh.edges.insert(mesh.edges.end(), *e);
		edge_map[&*e] = en;
	}
	for (FaceCRef f = faces.begin(); f != faces.end(); f++) {
		FaceRef fn = mesh.faces.insert(mesh.faces.end(), *f);
		face_map[&*f] = fn;
	}
	for (HalfedgeCRef h = halfedges.begin(); h != halfedges.end(); h++) {
		HalfedgeRef hn = mesh.halfedges.insert(mesh.halfedges.end(), *h);
		halfedge_map[&*h] = hn;
	}

	// "Search and replace" old pointers with new ones.
	for (HalfedgeRef he = mesh.halfedges.begin(); he != mesh.halfedges.end(); he++) {
		he->next = halfedge_map.at(&*he->next);
		he->twin = halfedge_map.at(&*he->twin);
		he->vertex = vertex_map.at(&*he->vertex);
		he->edge = edge_map.at(&*he->edge);
		he->face = face_map.at(&*he->face);
	}
	for (VertexRef v = mesh.vertices.begin(); v != mesh.vertices.end(); v++) {
		v->halfedge = halfedge_map.at(&*v->halfedge);
	}
	for (EdgeRef e = mesh.edges.begin(); e != mesh.edges.end(); e++) {
		e->halfedge = halfedge_map.at(&*e->halfedge);
	}
	for (FaceRef f = mesh.faces.begin(); f != mesh.faces.end(); f++) {
		f->halfedge = halfedge_map.at(&*f->halfedge);
	}

	return mesh;
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
		vertices.emplace_back(mesh.emplace_vertex());
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

		FaceRef face = mesh.emplace_face(boundary);
		HalfedgeRef prev = mesh.halfedges.end(); //keep track of previous edge around face to set next pointer
		for (uint32_t i = 0; i < loop.size(); ++i) {
			Index a = loop[i];
			Index b = loop[(i + 1) % loop.size()];
			assert(a != b);

			HalfedgeRef halfedge = mesh.emplace_halfedge();
			if (i == 0) face->halfedge = halfedge; //store first edge as face's halfedge pointer
			halfedge->vertex = vertices[a];

			//if first to mention vertex, set vertex's halfedge pointer:
			if (vertices[a]->halfedge == mesh.halfedges.end()) {
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
				EdgeRef edge = mesh.emplace_edge(false);
				halfedge->edge = edge;
				edge->halfedge = halfedge;
			} else {
				//found a twin -- connect twin pointers and reference its edge:
				assert(twin->second->twin == mesh.halfedges.end());
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
		if (halfedge->twin == mesh.halfedges.end()) {
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
	for (auto vertex = mesh.vertices.begin(); vertex != mesh.vertices.end(); ++vertex) {
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
	for (auto vertex = vertices.begin(); vertex != vertices.end(); ++vertex) {
		max_radius = std::max(max_radius, vertex->position.norm());
	}
	return max_radius;
}
