
#include <algorithm>
#include <imgui/imgui_custom.h>

#include "manager.h"
#include "model.h"
#include "widgets.h"

#include "../geometry/util.h"
#include "../platform/renderer.h"
#include "../scene/undo.h"

namespace Gui {

Model::Model()
	: vert_mesh(Util::sphere_mesh(0.05f, 1).to_gl()),
	  edge_mesh(Util::cyl_mesh_disjoint(0.05f, 1.0f).to_gl()),
	  halfedge_mesh(Util::arrow_mesh(0.05f, 0.1f, 1.0f).to_gl()) {
}

bool Model::mesh_expired() {
	return std::visit([](auto&& mesh) { return mesh.expired(); }, my_mesh);
}

Halfedge_Mesh& Model::get_mesh() {
	assert(!mesh_expired());
	return std::visit(
		overloaded{
			[](std::weak_ptr<Halfedge_Mesh> mesh) -> Halfedge_Mesh& { return *mesh.lock(); },
			[](std::weak_ptr<Skinned_Mesh> mesh) -> Halfedge_Mesh& { return mesh.lock()->mesh; }},
		my_mesh);
}

void Model::save_old_mesh() {
	if (mesh_expired()) return;
	old_mesh = std::visit(
		[](auto&& mesh) -> std::variant<Halfedge_Mesh, Skinned_Mesh> {
			return mesh.lock()->copy();
		},
		my_mesh);
}

void Model::load_old_mesh() {
	if (mesh_expired()) return;
	std::visit(overloaded{[this](std::weak_ptr<Halfedge_Mesh> mesh) {
							  *mesh.lock() = std::move(std::get<Halfedge_Mesh>(old_mesh));
						  },
	                      [this](std::weak_ptr<Skinned_Mesh> mesh) {
							  *mesh.lock() = std::move(std::get<Skinned_Mesh>(old_mesh));
						  }},
	           my_mesh);
}

void Model::undo_update_mesh(Undo& undo) {
	if (mesh_expired()) return;
	if (std::holds_alternative<Halfedge_Mesh>(old_mesh)) {
		undo.update_cached<Halfedge_Mesh>(mesh_name,
		                                  std::get<std::weak_ptr<Halfedge_Mesh>>(my_mesh),
		                                  std::move(std::get<Halfedge_Mesh>(old_mesh)));
	} else {
		undo.update_cached<Skinned_Mesh>(mesh_name, std::get<std::weak_ptr<Skinned_Mesh>>(my_mesh),
		                                 std::move(std::get<Skinned_Mesh>(old_mesh)));
	}
}

void Model::begin_transform() {

	if (mesh_expired()) return;

	save_old_mesh();

	auto elem = *selected_element();
	trans_begin = {};
	std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
							  trans_begin.verts = {vert->position};
							  trans_begin.center = vert->position;
						  },
	                      [&](Halfedge_Mesh::EdgeRef edge) {
							  trans_begin.center = edge->center();
							  trans_begin.verts = {edge->halfedge->vertex->position,
		                                           edge->halfedge->twin->vertex->position};
						  },
	                      [&](Halfedge_Mesh::FaceRef face) {
							  auto h = face->halfedge;
							  trans_begin.center = face->center();
							  do {
								  trans_begin.verts.push_back(h->vertex->position);
								  h = h->next;
							  } while (h != face->halfedge);
						  },
	                      [&](auto) {}},
	           elem);
}

void Model::update_vertex(Halfedge_Mesh::VertexRef vert) {

	auto get_inst = [&](auto e) { return screen_id_to_info[e->id + n_Widget_IDs].instance; };

	// Update current vertex
	float d;
	{
		GL::Instances::Info& info = spheres.get(get_inst(vert));
		vertex_viz(vert, d, info.transform);
		vert_sizes[vert->id] = d;
	}

	Halfedge_Mesh::HalfedgeRef h = vert->halfedge;

	// Update surrounding vertices & faces
	do {
		Halfedge_Mesh::VertexRef v = h->twin->vertex;
		GL::Instances::Info& vi = spheres.get(get_inst(v));
		vertex_viz(v, d, vi.transform);
		vert_sizes[v->id] = d;

		if (!h->face->boundary) {
			size_t idx = get_inst(h->face);
			face_viz(h->face, face_mesh.edit_verts(), face_mesh.edit_indices(), idx);

			Halfedge_Mesh::HalfedgeRef fh = h->face->halfedge;
			do {
				halfedge_viz(fh, arrows.get(get_inst(fh)).transform);
				fh = fh->next;
			} while (fh != h->face->halfedge);
		}

		h = h->twin->next;
	} while (h != vert->halfedge);

	// Update surrounding halfedges & edges
	do {
		if (!h->is_boundary()) {
			GL::Instances::Info& hi = arrows.get(get_inst(h));
			halfedge_viz(h, hi.transform);
		}
		if (!h->twin->is_boundary()) {
			GL::Instances::Info& thi = arrows.get(get_inst(h->twin));
			halfedge_viz(h->twin, thi.transform);
		}
		GL::Instances::Info& e = cylinders.get(get_inst(h->edge));
		edge_viz(h->edge, e.transform);

		h = h->twin->next;
	} while (h != vert->halfedge);
}

void Model::apply_transform(Widgets& widgets) {

	if (mesh_expired()) return;

	auto& mesh = get_mesh();
	auto elem = *selected_element();

	Widget_Type action = widgets.active;
	Transform delta = widgets.apply_action({});
	Vec3 abs_pos = trans_begin.center + delta.translation;

	std::visit(
		overloaded{
			[&](Halfedge_Mesh::VertexRef vert) {
				if (action == Widget_Type::move) {
					vert->position = abs_pos;
				}
				update_vertex(vert);
			},

			[&](Halfedge_Mesh::EdgeRef edge) {
				auto h = edge->halfedge;
				Vec3 v0 = trans_begin.verts[0];
				Vec3 v1 = trans_begin.verts[1];
				Vec3 center = trans_begin.center;

				if (action == Widget_Type::move) {
					Vec3 off = abs_pos - edge->center();
					h->vertex->position += off;
					h->twin->vertex->position += off;
				} else if (action == Widget_Type::rotate) {
					h->vertex->position = delta.rotation.rotate(v0 - center) + center;
					h->twin->vertex->position = delta.rotation.rotate(v1 - center) + center;
				} else if (action == Widget_Type::scale) {
					Mat4 s = Mat4::scale(delta.scale);
					h->vertex->position = s * (v0 - center) + center;
					h->twin->vertex->position = s * (v1 - center) + center;
				}
				update_vertex(edge->halfedge->vertex);
				update_vertex(edge->halfedge->twin->vertex);
			},

			[&](Halfedge_Mesh::FaceRef face) {
				auto h = face->halfedge;
				Vec3 center = trans_begin.center;

				if (action == Widget_Type::move) {
					Vec3 off = abs_pos - face->center();
					do {
						h->vertex->position += off;
						h = h->next;
					} while (h != face->halfedge);
				} else if (action == Widget_Type::rotate) {
					int i = 0;
					do {
						h->vertex->position =
							delta.rotation.rotate(trans_begin.verts[i] - center) + center;
						h = h->next;
						i++;
					} while (h != face->halfedge);
				} else if (action == Widget_Type::scale) {
					Mat4 s = Mat4::scale(delta.scale);
					int i = 0;
					do {
						h->vertex->position = s * (trans_begin.verts[i] - center) + center;
						h = h->next;
						i++;
					} while (h != face->halfedge);
				} else if (action == Widget_Type::bevel) {

					if (beveling == Bevel::vert) {
						mesh.bevel_vertex_positions(trans_begin.verts, face, delta.translation.x);
					} else if (beveling == Bevel::edge) {
						mesh.bevel_edge_positions(trans_begin.verts, face, delta.translation.x);
					} else {
						mesh.bevel_face_positions(trans_begin.verts, face, delta.translation.x,
				                                  delta.translation.y);
					}
				} else if (action == Widget_Type::extrude) {

					if (beveling == Bevel::face) {
						mesh.bevel_face_positions(trans_begin.verts, face, 0.0f,
				                                  delta.translation.y);
					}
				}

				h = face->halfedge;
				do {
					update_vertex(h->vertex);
					update_vertex(h->twin->next->twin->vertex);
					h = h->next;
				} while (h != face->halfedge);
			},

			[&](auto) {}},
		elem);
}

void Model::set_selected(Halfedge_Mesh::ElementRef elem) {

	std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
							  screen_selected_elem_id = vert->id + n_Widget_IDs;
						  },
	                      [&](Halfedge_Mesh::EdgeRef edge) {
							  screen_selected_elem_id = edge->id + n_Widget_IDs;
						  },
	                      [&](Halfedge_Mesh::FaceRef face) {
							  if (!face->boundary)
								  screen_selected_elem_id = face->id + n_Widget_IDs;
						  },
	                      [&](Halfedge_Mesh::HalfedgeRef halfedge) {
							  if (!halfedge->is_boundary())
								  screen_selected_elem_id = halfedge->id + n_Widget_IDs;
						  }},
	           elem);
}

std::tuple<GL::Instances&, GL::Instances&, GL::Instances&> Model::instances() {
	return {spheres, cylinders, arrows};
}

std::tuple<GL::Mesh&, GL::Mesh&, GL::Mesh&, GL::Mesh&> Model::meshes() {
	return {face_mesh, vert_mesh, edge_mesh, halfedge_mesh};
}

std::optional<Halfedge_Mesh::ElementRef> Model::selected_element() {

	if (mesh_expired()) return std::nullopt;

	rebuild();

	auto entry = screen_id_to_info.find(screen_selected_elem_id);
	if (entry == screen_id_to_info.end()) return std::nullopt;
	return entry->second.ref;
}

void Model::invalidate(const std::string& name) {
	if (name == mesh_name) needs_rebuild = true;
}

void Model::erase_mesh(const std::string& name) {
	if (name == mesh_name) {
		my_mesh = {};
		mesh_name = {};
		needs_rebuild = true;
	}
}

void Model::set_mesh(const std::string& name, std::weak_ptr<Halfedge_Mesh> mesh) {
	if (mesh_name != name) needs_rebuild = true;
	mesh_name = name;
	my_mesh = mesh;
}

void Model::set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh) {
	if (mesh_name != name) needs_rebuild = true;
	mesh_name = name;
	my_mesh = mesh;
}

void Model::vertex_viz(Halfedge_Mesh::VertexRef v, float& size, Mat4& transform) {

	// Sphere size ~ 0.05 * min incident edge length
	float min = FLT_MAX;
	float avg = 0.0f;
	size_t d = 0;

	auto he = v->halfedge;
	do {
		float len = he->edge->length();
		min = std::min(min, len);
		avg += len;
		d++;
		he = he->twin->next;
	} while (he != v->halfedge);

	avg = avg / d;
	size = std::clamp(min, avg / 10.0f, avg);

	transform = Mat4{Vec4{size, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, size, 0.0f, 0.0f},
	                 Vec4{0.0f, 0.0f, size, 0.0f}, Vec4{v->position, 1.0f}};
}

void Model::edge_viz(Halfedge_Mesh::EdgeRef e, Mat4& transform) {

	auto v_0 = e->halfedge->vertex;
	auto v_1 = e->halfedge->twin->vertex;
	Vec3 v0 = v_0->position;
	Vec3 v1 = v_1->position;

	Vec3 dir = v1 - v0;
	float l = dir.norm();
	dir /= l;

	// Cylinder width; 0.6 * min vertex scale
	float v0s = vert_sizes[v_0->id], v1s = vert_sizes[v_1->id];
	float s = 0.5f * std::min(v0s, v1s);

	if (1.0f - std::abs(dir.y) < EPS_F) {
		l *= sign(dir.y);
		transform = Mat4{Vec4{s, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, l, 0.0f, 0.0f},
		                 Vec4{0.0f, 0.0f, s, 0.0f}, Vec4{v0, 1.0f}};
	} else {
		Vec3 x = cross(dir, Vec3{0.0f, 1.0f, 0.0f}).unit();
		Vec3 z = cross(x, dir).unit();
		transform = Mat4{Vec4{x * s, 0.0f}, Vec4{dir * l, 0.0f}, Vec4{z * s, 0.0f}, Vec4{v0, 1.0f}};
	}
}

void Model::halfedge_viz(Halfedge_Mesh::HalfedgeRef h, Mat4& transform) {

	auto v_0 = h->vertex;
	auto v_1 = h->twin->vertex;
	Vec3 v0 = v_0->position;
	Vec3 v1 = v_1->position;

	Vec3 dir = v1 - v0;
	float l = dir.norm();
	dir /= l;
	l *= 0.6f;
	// Same width as edge

	float v0s = vert_sizes[v_0->id], v1s = vert_sizes[v_1->id];
	float s = 0.3f * (v0s < v1s ? v0s : v1s);

	// Move to center of edge and towards center of face
	Vec3 offset = (v1 - v0) * 0.2f;
	auto base = h->face->halfedge;

	if (base->vertex == v_0) {
		base = h->next->next;
	} else if (base->vertex == v_1) {
		Halfedge_Mesh::HalfedgeRef hf = h;
		do {
			hf = hf->next;
		} while (hf->next != h);
		base = hf;
	}

	Vec3 face_n;
	do {
		Vec3 b = base->vertex->position;
		base = base->next;
		face_n = cross(b - v0, b - v1);
	} while (face_n.norm() < EPS_F && base != h->face->halfedge);

	offset += cross(face_n.unit(), dir).unit() * s * 0.2f;

	// Align edge
	if (1.0f - std::abs(dir.y) < EPS_F) {
		l *= sign(dir.y);
		transform = Mat4{Vec4{s, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, l, 0.0f, 0.0f},
		                 Vec4{0.0f, 0.0f, s, 0.0f}, Vec4{v0 + offset, 1.0f}};
	} else {
		Vec3 x = cross(dir, Vec3{0.0f, 1.0f, 0.0f}).unit();
		Vec3 z = cross(x, dir).unit();
		transform = Mat4{Vec4{x * s, 0.0f}, Vec4{dir * l, 0.0f}, Vec4{z * s, 0.0f},
		                 Vec4{v0 + offset, 1.0f}};
	}
}

void Model::face_viz(Halfedge_Mesh::FaceRef face, std::vector<GL::Mesh::Vert>& verts,
                     std::vector<GL::Mesh::Index>& idxs, size_t insert_at) {

	std::vector<GL::Mesh::Vert> face_verts;
	uint32_t id = face->id + n_Widget_IDs;

	Halfedge_Mesh::HalfedgeRef h = face->halfedge;
	do {
		Halfedge_Mesh::VertexRef v = h->vertex;
		face_verts.push_back({v->position, h->corner_normal, h->corner_uv, 0});
		h = h->next;
	} while (h != face->halfedge);

	screen_id_to_info[id] = {face, insert_at};

	if (face_verts.size() < 3) return;

	size_t max = insert_at + (face_verts.size() - 2) * 3;
	if (verts.size() < max) verts.resize(max);
	if (idxs.size() < max) idxs.resize(max);

	Vec3 v0 = face_verts[0].pos;
	Vec3 n0 = face_verts[0].norm;
	Vec2 uv0 = face_verts[0].uv;

	for (size_t i = 1; i <= face_verts.size() - 2; i++) {
		Vec3 v1 = face_verts[i].pos;
		Vec3 v2 = face_verts[i + 1].pos;
		Vec2 uv1 = face_verts[i].uv;
		Vec2 uv2 = face_verts[i + 1].uv;
		Vec3 n1 = face_verts[i].norm;
		Vec3 n2 = face_verts[i + 1].norm;

		idxs[insert_at] = static_cast<GL::Mesh::Index>(insert_at);
		verts[insert_at++] = {v0, n0, uv0, id};

		idxs[insert_at] = static_cast<GL::Mesh::Index>(insert_at);
		verts[insert_at++] = {v1, n1, uv1, id};

		idxs[insert_at] = static_cast<GL::Mesh::Index>(insert_at);
		verts[insert_at++] = {v2, n2, uv2, id};
	}
}

void Model::rebuild() {

	if (!needs_rebuild) return;
	needs_rebuild = false;

	if (mesh_expired()) return;
	auto& mesh = get_mesh();

	screen_id_to_info.clear();
	vert_sizes.clear();

	std::vector<GL::Mesh::Vert> verts;
	std::vector<GL::Mesh::Index> idxs;

	for (auto f = mesh.faces_begin(); f != mesh.faces_end(); f++) {
		if (!f->boundary) face_viz(f, verts, idxs, verts.size());
	}
	face_mesh.recreate(std::move(verts), std::move(idxs));

	// Create sphere for each vertex
	spheres.clear();
	for (auto v = mesh.vertices_begin(); v != mesh.vertices_end(); v++) {

		float d;
		Mat4 transform;
		vertex_viz(v, d, transform);
		vert_sizes[v->id] = d;

		uint32_t id = v->id + n_Widget_IDs;
		screen_id_to_info[id] = {v, spheres.add(transform, id)};
	}

	// Create cylinder for each edge
	cylinders.clear();
	for (auto e = mesh.edges_begin(); e != mesh.edges_end(); e++) {

		// We don't want to render edges between two boundary faces, since the boundaries
		// should look contiguous
		if (e->halfedge->is_boundary() && e->halfedge->twin->is_boundary()) {

			// Unless both surrounding boundaries are the same face, in which case we should
			// render this edge to show that the next vertex is connected
			if (e->halfedge->face != e->halfedge->twin->face) continue;
		}

		Mat4 transform;
		edge_viz(e, transform);

		uint32_t id = e->id + n_Widget_IDs;
		screen_id_to_info[id] = {e, cylinders.add(transform, id)};
	}

	// Create arrow for each halfedge
	arrows.clear();
	for (auto h = mesh.halfedges_begin(); h != mesh.halfedges_end(); h++) {

		if (h->is_boundary()) continue;

		Mat4 transform;
		halfedge_viz(h, transform);

		uint32_t id = h->id + n_Widget_IDs;
		screen_id_to_info[id] = {h, arrows.add(transform, id)};
	}

	validate();
}

bool Model::begin_bevel(std::string& err) {

	if (mesh_expired()) return false;
	auto& mesh = get_mesh();

	auto sel = selected_element();
	if (!sel.has_value()) return false;

	save_old_mesh();

	auto new_face = std::visit(
		overloaded{[&](Halfedge_Mesh::VertexRef vert) {
					   beveling = Bevel::vert;
					   return mesh.bevel_vertex(vert);
				   },
	               [&](Halfedge_Mesh::EdgeRef edge) {
					   beveling = Bevel::edge;
					   return mesh.bevel_edge(edge);
				   },
	               [&](Halfedge_Mesh::FaceRef face) {
					   beveling = Bevel::face;
					   return mesh.bevel_face(face);
				   },
	               [&](auto) -> std::optional<Halfedge_Mesh::FaceRef> { return std::nullopt; }},
		*sel);

	err = validate();
	if (!err.empty() || !new_face.has_value()) {
		load_old_mesh();
		needs_rebuild = true;
		return false;
	}

	Halfedge_Mesh::FaceRef face = new_face.value();

	needs_rebuild = true;
	set_selected(face);

	trans_begin = {};
	auto h = face->halfedge;
	trans_begin.center = face->center();
	do {
		trans_begin.verts.push_back(h->vertex->position);
		h = h->next;
	} while (h != face->halfedge);

	return true;
}

bool Model::begin_extrude(std::string& err) {

	if (mesh_expired()) return false;
	auto& mesh = get_mesh();

	auto sel = selected_element();
	if (!sel.has_value()) return false;
	Halfedge_Mesh::FaceRef f;

	save_old_mesh();

	std::optional<Halfedge_Mesh::ElementRef> new_obj = std::visit(
		overloaded{[&](Halfedge_Mesh::FaceRef face) {
					   beveling = Bevel::face;
					   return std::optional<Halfedge_Mesh::ElementRef>{mesh.bevel_face(face)};
				   },
	               [&](auto) -> std::optional<Halfedge_Mesh::ElementRef> { return std::nullopt; }},
		*sel);

	err = validate();
	if (!err.empty() || !new_obj.has_value()) {
		load_old_mesh();
		needs_rebuild = true;
		return false;
	}
	Halfedge_Mesh::ElementRef elem = new_obj.value();

	return std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
									 set_selected(vert);
									 needs_rebuild = true;
									 trans_begin = {};
									 trans_begin.verts.push_back(vert->position);
									 return true;
								 },
	                             [&](Halfedge_Mesh::FaceRef face) {
									 set_selected(face);
									 needs_rebuild = true;
									 trans_begin = {};
									 auto h = face->halfedge;
									 trans_begin.center = face->center();
									 do {
										 trans_begin.verts.push_back(h->vertex->position);
										 h = h->next;
									 } while (h != face->halfedge);

									 return true;
								 },
	                             [](auto) -> bool { return false; }},
	                  elem);
}

bool Model::keydown(Widgets& widgets, SDL_Keysym key, View_3D& cam) {

	auto sel = selected_element();
	if (!sel.has_value()) return false;

	if (std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(*sel)) {
		auto h = std::get<Halfedge_Mesh::HalfedgeRef>(*sel);
		if (key.sym == SDLK_n) {
			set_selected(h->next);
			return true;
		} else if (key.sym == SDLK_t) {
			set_selected(h->twin);
			return true;
		} else if (key.sym == SDLK_v) {
			set_selected(h->vertex);
			return true;
		} else if (key.sym == SDLK_e) {
			set_selected(h->edge);
			return true;
		} else if (key.sym == SDLK_f) {
			set_selected(h->face);
			return true;
		}
	}

	switch (key.sym) {
	case SDLK_b: widgets.active = Widget_Type::bevel; return true;
	case SDLK_e: widgets.active = Widget_Type::extrude; return true;
	case SDLK_c: zoom_to(*sel, cam); return true;
	case SDLK_h: {
		std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) { set_selected(vert->halfedge); },
		                      [&](Halfedge_Mesh::EdgeRef edge) { set_selected(edge->halfedge); },
		                      [&](Halfedge_Mesh::FaceRef face) { set_selected(face->halfedge); },
		                      [&](auto) {}},
		           *sel);
		return true;
	}
	}

	return false;
}

template<typename T>
std::string Model::update_mesh(Undo& undo, Halfedge_Mesh::ElementRef ref, T&& op) {

	if (mesh_expired()) return {};
	auto& mesh = get_mesh();

	std::optional<Halfedge_Mesh::ElementRef> new_ref = op(mesh, ref);

	auto err = validate();
	if (!err.empty() || !new_ref.has_value()) {
		load_old_mesh();
		needs_rebuild = true;
	} else {
		set_selected(new_ref.value());
		undo_update_mesh(undo);
	}

	return err;
}

template<typename T> std::string Model::update_mesh_global(Undo& undo, T&& op) {

	if (mesh_expired()) return {};
	auto& mesh = get_mesh();

	bool success = op(mesh);
	needs_rebuild = true;

	auto err = validate();
	if (!err.empty() || !success) {
		load_old_mesh();
	} else {
		screen_selected_elem_id = 0;
		screen_hovered_elem_id = 0;
		undo_update_mesh(undo);
	}
	return err;
}

std::string Model::validate() {

	if (mesh_expired()) return {};
	auto& mesh = get_mesh();

	auto valid = mesh.validate();
	if (valid.has_value()) {
		auto& msg = valid.value();
		screen_err_id = Halfedge_Mesh::id_of(msg.first) + n_Widget_IDs;
		err_msg = msg.second;
		return msg.second;
	}

	return {};
}

void Model::zoom_to(Halfedge_Mesh::ElementRef ref, View_3D& cam) {

	if (mesh_expired()) return;
	auto& mesh = get_mesh();

	float d = cam.dist();
	Vec3 center = Halfedge_Mesh::center_of(ref);
	Vec3 normal = mesh.normal_of(ref);

	if (center.valid() && normal.valid()) {
		Vec3 pos = center + normal * d;
		cam.look_at(center, pos);
	}
}

std::string Model::ui_sidebar(Scene& scene, Undo& undo, Widgets& widgets, View_3D& camera) {

	using namespace ImGui;

	if (CollapsingHeader("Edit Colors")) {
		ColorEdit3("Face", f_col.data);
		ColorEdit3("Vertex", v_col.data);
		ColorEdit3("Edge", e_col.data);
		ColorEdit3("Halfedge", he_col.data);
	}

	auto bullet = [&](const std::string& name, auto& mesh) {
		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Bullet |
		                                ImGuiTreeNodeFlags_NoTreePushOnOpen |
		                                ImGuiTreeNodeFlags_SpanAvailWidth;
		bool is_selected = name == mesh_name;
		if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

		TreeNodeEx(name.c_str(), node_flags, "%s", name.c_str());
		if (IsItemClicked()) {
			mesh_name = name;
			my_mesh = mesh;
			needs_rebuild = true;
		}
	};
	if (CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (auto& [name, mesh] : scene.meshes) {
			bullet(name, mesh);
		}
		for (auto& [name, mesh] : scene.skinned_meshes) {
			bullet(name, mesh);
		}
	}

	if (scene.get<Halfedge_Mesh>(mesh_name).expired() &&
	    scene.get<Skinned_Mesh>(mesh_name).expired()) {
		my_mesh = {};
		mesh_name = {};
		needs_rebuild = true;
	}

	if (mesh_expired()) return {};

	Separator();
	Text("Edit Mesh");
	if (std::holds_alternative<std::weak_ptr<Halfedge_Mesh>>(my_mesh)) {
		he_edit_widget.ui(mesh_name, undo, std::get<std::weak_ptr<Halfedge_Mesh>>(my_mesh));
	} else {
		skin_edit_widget.ui(mesh_name, undo, std::get<std::weak_ptr<Skinned_Mesh>>(my_mesh));
	}

	Separator();
	Text("Global Operations");
	if (Button("Linear")) {
		save_old_mesh();
		return update_mesh_global(undo, [](Halfedge_Mesh& m) { return m.subdivide(SubD::linear); });
	}
	if (WrapButton("Catmull-Clark")) {
		save_old_mesh();
		return update_mesh_global(
			undo, [](Halfedge_Mesh& m) { return m.subdivide(SubD::catmull_clark); });
	}
	if (WrapButton("Loop")) {
		save_old_mesh();
		return update_mesh_global(undo, [](Halfedge_Mesh& m) { return m.subdivide(SubD::loop); });
	}
	if (Button("Triangulate")) {
		save_old_mesh();
		return update_mesh_global(undo, [](Halfedge_Mesh& m) {
			m.triangulate();
			return true;
		});
	}
	if (WrapButton("Remesh")) {
		save_old_mesh();
		return update_mesh_global(undo, [](Halfedge_Mesh& m) { return m.isotropic_remesh(3, 10); });
	}
	if (WrapButton("Simplify")) {
		save_old_mesh();
		return update_mesh_global(undo, [](Halfedge_Mesh& m) { return m.simplify(0.25f); });
	}

	{
		auto sel = selected_element();
		if (sel.has_value()) {

			Text("Local Operations");
			widgets.action_button(Widget_Type::move, "Move [m]", false);
			widgets.action_button(Widget_Type::rotate, "Rotate [r]");
			widgets.action_button(Widget_Type::scale, "Scale [s]");
			widgets.action_button(Widget_Type::bevel, "Bevel [b]");
			widgets.action_button(Widget_Type::extrude, "Extrude [e]");
			std::string err = std::visit(
				overloaded{
					[&](Halfedge_Mesh::VertexRef vert) -> std::string {
						if (Button("Erase [del]")) {
							save_old_mesh();
							return update_mesh(
								undo, vert, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef vert) {
									return m.erase_vertex(std::get<Halfedge_Mesh::VertexRef>(vert));
								});
						}
						return {};
					},
					[&](Halfedge_Mesh::EdgeRef edge) -> std::string {
						if (Button("Erase [del]")) {
							save_old_mesh();
							return update_mesh(
								undo, edge, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
									return m.erase_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
								});
						}
						if (WrapButton("Collapse")) {
							save_old_mesh();
							return update_mesh(
								undo, edge, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
									return m.collapse_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
								});
						}
						if (WrapButton("Flip")) {
							save_old_mesh();
							return update_mesh(
								undo, edge, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
									return m.flip_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
								});
						}
						if (WrapButton("Split")) {
							save_old_mesh();
							return update_mesh(
								undo, edge, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
									return m.split_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
								});
						}
						if (WrapButton("Bisect")) {
							save_old_mesh();
							return update_mesh(
								undo, edge, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
									return m.bisect_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
								});
						}
						return {};
					},
					[&](Halfedge_Mesh::FaceRef face) -> std::string {
						if (Button("Collapse")) {
							save_old_mesh();
							return update_mesh(
								undo, face, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef face) {
									return m.collapse_face(std::get<Halfedge_Mesh::FaceRef>(face));
								});
						}
						if (Button("Inset Vertex")) {
							save_old_mesh();
							return update_mesh(
								undo, face, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef face) {
									return m.inset_vertex(std::get<Halfedge_Mesh::FaceRef>(face));
								});
						}
						return {};
					},
					[&](auto) -> std::string { return {}; }},
				*sel);

			if (!err.empty()) return err;
		}
	}

	{
		auto sel = selected_element();
		if (sel.has_value()) {
			Separator();
			Text("Navigation");

			if (Button("Center Camera [c]")) {
				zoom_to(*sel, camera);
			}
			std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
									  if (Button("Halfedge [h]")) {
										  set_selected(vert->halfedge);
									  }
								  },
			                      [&](Halfedge_Mesh::EdgeRef edge) {
									  if (Button("Halfedge [h]")) {
										  set_selected(edge->halfedge);
									  }
								  },
			                      [&](Halfedge_Mesh::FaceRef face) {
									  if (Button("Halfedge [h]")) {
										  set_selected(face->halfedge);
									  }
								  },
			                      [&](Halfedge_Mesh::HalfedgeRef halfedge) {
									  if (Button("Vertex [v]")) {
										  set_selected(halfedge->vertex);
									  }
									  if (WrapButton("Edge [e]")) {
										  set_selected(halfedge->edge);
									  }
									  if (WrapButton("Face [f]")) {
										  set_selected(halfedge->face);
									  }
									  if (WrapButton("Twin [t]")) {
										  set_selected(halfedge->twin);
									  }
									  if (WrapButton("Next [n]")) {
										  set_selected(halfedge->next);
									  }
								  }},
			           *sel);
		}
	}

	{
		auto sel = selected_element();
		if (sel.has_value()) {
			Separator();
			Text("ID Info");

			Text("Selected: %u", Halfedge_Mesh::id_of(*sel));
			std::visit(
				overloaded{
					[&](Halfedge_Mesh::VertexRef vert) {
						Text("Halfedge: %u", vert->halfedge->id);
					},
					[&](Halfedge_Mesh::EdgeRef edge) { Text("Halfedge: %u", edge->halfedge->id); },
					[&](Halfedge_Mesh::FaceRef face) { Text("Halfedge: %u", face->halfedge->id); },
					[&](Halfedge_Mesh::HalfedgeRef halfedge) {
						Text("Vertex: %u", halfedge->vertex->id);
						Text("Edge: %u", halfedge->edge->id);
						Text("Face: %u", halfedge->face->id);
						Text("Twin: %u", halfedge->twin->id);
						Text("Next: %u", halfedge->next->id);
					}},
				*sel);
		}
	}

	if (!err_msg.empty()) {
		Separator();
		Spectrum red = Gui::Color::red;
		TextColored(ImVec4{red.r, red.g, red.b, 1.0f}, "Error");
		TextWrapped("%s", err_msg.c_str());
		TextWrapped("(Your operation resulted in an invalid mesh.)");
		if (Button("Select Error")) {
			screen_selected_elem_id = screen_err_id;
		}
		if (WrapButton("Clear")) {
			screen_err_id = 0;
			err_msg = {};
		}
	}

	Separator();
	return {};
}

void Model::erase_selected(Undo& undo) {

	if (mesh_expired()) return;

	auto sel_ = selected_element();
	if (!sel_.has_value()) return;

	Halfedge_Mesh::ElementRef sel = sel_.value();

	save_old_mesh();

	std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
							  return update_mesh(
								  undo, vert, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef vert) {
									  return m.erase_vertex(
										  std::get<Halfedge_Mesh::VertexRef>(vert));
								  });
						  },
	                      [&](Halfedge_Mesh::EdgeRef edge) {
							  return update_mesh(
								  undo, edge, [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
									  return m.erase_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
								  });
						  },
	                      [](auto) { return std::string{}; }},
	           sel);
}

void Model::clear_select() {
	screen_selected_elem_id = 0;
}

void Model::render(Widgets& widgets, View_3D& cam) {

	if (mesh_expired()) return;

	rebuild();

	Mat4 view = cam.get_view();

	Renderer::HalfedgeOpt opts(*this);
	opts.modelview = view;
	opts.v_color = v_col;
	opts.f_color = f_col;
	opts.e_color = e_col;
	opts.he_color = he_col;
	opts.err_color = err_col;
	opts.err_id = screen_err_id;
	opts.sel_id = screen_selected_elem_id;
	opts.hov_id = screen_hovered_elem_id;
	Renderer::get().halfedge_editor(opts);

	auto elem = selected_element();
	if (elem.has_value()) {
		auto e = *elem;
		Vec3 pos = Halfedge_Mesh::center_of(e);
		if (!std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(e)) {
			float scl = std::min((cam.pos() - pos).norm() / 5.5f, 10.0f);
			widgets.render(view, pos, scl);
		}
	}
}

std::string Model::end_transform(Undo& undo) {

	if (mesh_expired()) return {};

	auto err = validate();
	if (!err.empty()) {
		load_old_mesh();
		needs_rebuild = true;
	} else {
		undo_update_mesh(undo);
	}
	return err;
}

Vec3 Model::selected_pos() {
	auto elem = selected_element();
	assert(elem.has_value());
	return Halfedge_Mesh::center_of(*elem);
}

std::string Model::select(Widgets& widgets, uint32_t screen_id, Vec3 cam, Vec2 spos, Vec3 dir) {

	if (screen_id && widgets.active == Widget_Type::bevel && screen_id == screen_selected_elem_id) {

		std::string err;
		if (!begin_bevel(err)) {
			widgets.end_drag();
			return err;
		} else {
			widgets.start_drag(Halfedge_Mesh::center_of(selected_element().value()), cam, spos,
			                   dir);
			apply_transform(widgets);
		}

	} else if (screen_id && widgets.active == Widget_Type::extrude &&
	           screen_id == screen_selected_elem_id) {

		std::string err;
		if (!begin_extrude(err)) {
			widgets.end_drag();
			return err;
		} else {
			std::visit(
				overloaded{[&](Halfedge_Mesh::VertexRef vert) {
							   widgets.start_drag(Halfedge_Mesh::center_of(vert), cam, spos, dir);
							   apply_transform(widgets);
						   },
			               [&](Halfedge_Mesh::FaceRef face) {
							   widgets.start_drag(Halfedge_Mesh::center_of(face), cam, spos, dir);
							   apply_transform(widgets);
						   },
			               [](auto) { return; }},
				selected_element().value());
		}

	} else if (!widgets.is_dragging() && screen_id >= n_Widget_IDs) {
		screen_selected_elem_id = screen_id;
	} else if (!screen_id) {
		screen_selected_elem_id = 0;
	}

	if (widgets.want_drag()) {
		auto e = selected_element();
		if (e.has_value() && !std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(*e)) {
			widgets.start_drag(Halfedge_Mesh::center_of(*e), cam, spos, dir);
			if (widgets.active != Widget_Type::bevel && widgets.active != Widget_Type::extrude) {
				begin_transform();
			}
		}
	}
	return {};
}

void Model::hover(uint32_t id) {
	screen_hovered_elem_id = id;
}

} // namespace Gui
