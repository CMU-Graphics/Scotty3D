
#include "indexed.h"
#include "halfedge.h"

Indexed_Mesh Indexed_Mesh::from_halfedge_mesh(Halfedge_Mesh const &halfedge_mesh, SplitOrAverage split_or_average) {
	
	std::vector<Indexed_Mesh::Vert> verts;
	std::vector<Indexed_Mesh::Index> idxs;

	if (split_or_average == SplitEdges) {
		for (Halfedge_Mesh::FaceCRef f = halfedge_mesh.faces.begin(); f != halfedge_mesh.faces.end(); f++) {
			if (f->boundary) continue;

			//every corner gets its own copy of a vertex:
			uint32_t corners_begin = static_cast<uint32_t>(verts.size());
			Halfedge_Mesh::HalfedgeCRef h = f->halfedge;
			do {
				Vert vert;
				vert.pos = h->vertex->position;
				vert.norm = h->corner_normal;
				vert.uv = h->corner_uv;
				vert.id = f->id;
				verts.emplace_back(vert);
				h = h->next;
			} while (h != f->halfedge);
			uint32_t corners_end = static_cast<uint32_t>(verts.size());

			//divide face into a triangle fan:
			for (size_t i = corners_begin + 1; i + 1 < corners_end; i++) {
				idxs.emplace_back(corners_begin);
				idxs.emplace_back(static_cast<uint32_t>(i));
				idxs.emplace_back(static_cast<uint32_t>(i+1));
			}
		}
	} else if (split_or_average == AverageData) {
		std::unordered_map< Halfedge_Mesh::VertexCRef, Index > vref_to_index;
		std::vector< uint32_t > corners_at_vertex;
		corners_at_vertex.reserve(halfedge_mesh.vertices.size());
		verts.reserve(halfedge_mesh.vertices.size());
		for (Halfedge_Mesh::VertexCRef vertex = halfedge_mesh.vertices.begin(); vertex != halfedge_mesh.vertices.end(); ++vertex) {
			Vert vert;
			vert.pos = vertex->position;
			vert.norm = Vec3(0.0f, 0.0f, 0.0f);
			vert.uv = Vec2(0.0f, 0.0f);
			vert.id = vertex->id;
			verts.emplace_back(vert);
			corners_at_vertex.emplace_back(0);
			auto ret = vref_to_index.emplace(vertex, static_cast<Index>(verts.size()-1));
			assert(ret.second); //vertex reference is certainly unique
		}

		for (Halfedge_Mesh::FaceCRef f = halfedge_mesh.faces.begin(); f != halfedge_mesh.faces.end(); f++) {
			if (f->boundary) continue;

			std::vector<Index> face_verts;
			Halfedge_Mesh::HalfedgeCRef h = f->halfedge;
			do {
				auto idx_entry = vref_to_index.find(h->vertex);
				assert(idx_entry != vref_to_index.end()); //mesh faces must only reference vertices in the mesh
				uint32_t vertex_index = idx_entry->second;

				//record corner in face:
				face_verts.emplace_back(vertex_index);

				//add corner to average values:
				verts[vertex_index].norm += h->corner_normal;
				verts[vertex_index].uv += h->corner_uv;
				corners_at_vertex[vertex_index] += 1;

				h = h->next;
			} while (h != f->halfedge);

			assert(face_verts.size() >= 3);
			for (size_t j = 1; j + 1 < face_verts.size(); j++) {
				idxs.emplace_back(face_verts[0]);
				idxs.emplace_back(face_verts[j]);
				idxs.emplace_back(face_verts[j+1]);
			}
		}
		
		//actually average values at the corners:
		for (uint32_t v = 0; v < verts.size(); ++v) {
			if (corners_at_vertex[v] > 1) {
				verts[v].norm = verts[v].norm.unit();
				verts[v].uv /= static_cast<float>(corners_at_vertex[v]);
			}
		}
	} else {
		assert(0 && "No other options available for split_or_average parameter.");
	}

	return Indexed_Mesh(std::move(verts), std::move(idxs));

}


Indexed_Mesh::Indexed_Mesh(std::vector<Vert>&& vertices, std::vector<Index>&& indices)
	: vs(std::move(vertices)), is(std::move(indices)) {
	assert(is.size() % 3 == 0);
}

std::vector<Indexed_Mesh::Vert>& Indexed_Mesh::vertices() {
	return vs;
}

std::vector<Indexed_Mesh::Index>& Indexed_Mesh::indices() {
	return is;
}

const std::vector<Indexed_Mesh::Vert>& Indexed_Mesh::vertices() const {
	return vs;
}

const std::vector<Indexed_Mesh::Index>& Indexed_Mesh::indices() const {
	return is;
}

uint32_t Indexed_Mesh::tris() const {
	assert(is.size() % 3 == 0);
	return static_cast<uint32_t>(is.size() / 3);
}

GL::Mesh Indexed_Mesh::to_gl() const {
	std::vector<GL::Mesh::Vert> verts;
	std::vector<GL::Mesh::Index> inds;
	verts.reserve(vs.size());
	inds.reserve(is.size());
	for (uint32_t i = 0; i < vs.size(); ++i) {
		verts.push_back({vs[i].pos, vs[i].norm, vs[i].uv, vs[i].id});
	}
	for (uint32_t i = 0; i < is.size(); ++i) {
		inds.push_back(is[i]);
	}
	return GL::Mesh(std::move(verts), std::move(inds));
}

Indexed_Mesh Indexed_Mesh::copy() const {
	auto v2 = vs;
	auto i2 = is;
	return Indexed_Mesh(std::move(v2), std::move(i2));
}
