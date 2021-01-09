
#include <algorithm>
#include <imgui/imgui.h>

#include "manager.h"
#include "model.h"
#include "widgets.h"

#include "../geometry/util.h"
#include "../scene/renderer.h"
#include "../scene/undo.h"

namespace Gui {

Model::Model()
    : spheres(Util::sphere_mesh(0.05f, 1)), cylinders(Util::cyl_mesh(0.05f, 1.0f)),
      arrows(Util::arrow_mesh(0.05f, 0.1f, 1.0f)) {
}

void Model::begin_transform() {

    my_mesh->copy_to(old_mesh);

    auto elem = *selected_element();
    trans_begin = {};
    std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
                              trans_begin.verts = {vert->pos};
                              trans_begin.center = vert->pos;
                          },
                          [&](Halfedge_Mesh::EdgeRef edge) {
                              trans_begin.center = edge->center();
                              trans_begin.verts = {edge->halfedge()->vertex()->pos,
                                                   edge->halfedge()->twin()->vertex()->pos};
                          },
                          [&](Halfedge_Mesh::FaceRef face) {
                              auto h = face->halfedge();
                              trans_begin.center = face->center();
                              do {
                                  trans_begin.verts.push_back(h->vertex()->pos);
                                  h = h->next();
                              } while(h != face->halfedge());
                          },
                          [&](auto) {}},
               elem);
}

void Model::update_vertex(Halfedge_Mesh::VertexRef vert) {

    // Update current vertex
    float d;
    {
        GL::Instances::Info& info = spheres.get(id_to_info[vert->id()].instance);
        vertex_viz(vert, d, info.transform);
        vert_sizes[vert->id()] = d;
    }

    Halfedge_Mesh::HalfedgeRef h = vert->halfedge();

    // Update surrounding vertices & faces
    do {
        Halfedge_Mesh::VertexRef v = h->twin()->vertex();
        GL::Instances::Info& vi = spheres.get(id_to_info[v->id()].instance);
        vertex_viz(v, d, vi.transform);
        vert_sizes[v->id()] = d;

        if(!h->face()->is_boundary()) {
            size_t idx = id_to_info[h->face()->id()].instance;
            face_viz(h->face(), face_mesh.edit_verts(), face_mesh.edit_indices(), idx);

            Halfedge_Mesh::HalfedgeRef fh = h->face()->halfedge();
            do {
                halfedge_viz(fh, arrows.get(id_to_info[fh->id()].instance).transform);
                fh = fh->next();
            } while(fh != h->face()->halfedge());
        }

        h = h->twin()->next();
    } while(h != vert->halfedge());

    // Update surrounding halfedges & edges
    do {
        if(!h->is_boundary()) {
            GL::Instances::Info& hi = arrows.get(id_to_info[h->id()].instance);
            halfedge_viz(h, hi.transform);
        }
        if(!h->twin()->is_boundary()) {
            GL::Instances::Info& thi = arrows.get(id_to_info[h->twin()->id()].instance);
            halfedge_viz(h->twin(), thi.transform);
        }
        GL::Instances::Info& e = cylinders.get(id_to_info[h->edge()->id()].instance);
        edge_viz(h->edge(), e.transform);

        h = h->twin()->next();
    } while(h != vert->halfedge());
}

void Model::apply_transform(Widgets& widgets) {

    auto elem = *selected_element();

    Widget_Type action = widgets.active;
    Pose delta = widgets.apply_action({});
    Vec3 abs_pos = trans_begin.center + delta.pos;

    std::visit(
        overloaded{[&](Halfedge_Mesh::VertexRef vert) {
                       if(action == Widget_Type::move) {
                           vert->pos = abs_pos;
                       }
                       update_vertex(vert);
                   },

                   [&](Halfedge_Mesh::EdgeRef edge) {
                       auto h = edge->halfedge();
                       Vec3 v0 = trans_begin.verts[0];
                       Vec3 v1 = trans_begin.verts[1];
                       Vec3 center = trans_begin.center;

                       if(action == Widget_Type::move) {
                           Vec3 off = abs_pos - edge->center();
                           h->vertex()->pos += off;
                           h->twin()->vertex()->pos += off;
                       } else if(action == Widget_Type::rotate) {
                           Quat q = Quat::euler(delta.euler);
                           h->vertex()->pos = q.rotate(v0 - center) + center;
                           h->twin()->vertex()->pos = q.rotate(v1 - center) + center;
                       } else if(action == Widget_Type::scale) {
                           Mat4 s = Mat4::scale(delta.scale);
                           h->vertex()->pos = s * (v0 - center) + center;
                           h->twin()->vertex()->pos = s * (v1 - center) + center;
                       }
                       update_vertex(edge->halfedge()->vertex());
                       update_vertex(edge->halfedge()->twin()->vertex());
                   },

                   [&](Halfedge_Mesh::FaceRef face) {
                       auto h = face->halfedge();
                       Vec3 center = trans_begin.center;

                       if(action == Widget_Type::move) {
                           Vec3 off = abs_pos - face->center();
                           do {
                               h->vertex()->pos += off;
                               h = h->next();
                           } while(h != face->halfedge());
                       } else if(action == Widget_Type::rotate) {
                           Quat q = Quat::euler(delta.euler);
                           int i = 0;
                           do {
                               h->vertex()->pos = q.rotate(trans_begin.verts[i] - center) + center;
                               h = h->next();
                               i++;
                           } while(h != face->halfedge());
                       } else if(action == Widget_Type::scale) {
                           Mat4 s = Mat4::scale(delta.scale);
                           int i = 0;
                           do {
                               h->vertex()->pos = s * (trans_begin.verts[i] - center) + center;
                               h = h->next();
                               i++;
                           } while(h != face->halfedge());
                       } else if(action == Widget_Type::bevel) {

                           if(beveling == Bevel::vert) {
                               my_mesh->bevel_vertex_positions(trans_begin.verts, face,
                                                               delta.pos.x);
                           } else if(beveling == Bevel::edge) {
                               my_mesh->bevel_edge_positions(trans_begin.verts, face, delta.pos.x);
                           } else {
                               my_mesh->bevel_face_positions(trans_begin.verts, face, delta.pos.x,
                                                             delta.pos.y);
                           }
                       }

                       h = face->halfedge();
                       do {
                           update_vertex(h->vertex());
                           update_vertex(h->twin()->next()->twin()->vertex());
                           h = h->next();
                       } while(h != face->halfedge());
                   },

                   [&](auto) {}},
        elem);
}

void Model::set_selected(Halfedge_Mesh::ElementRef elem) {

    std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) { selected_elem_id = vert->id(); },
                          [&](Halfedge_Mesh::EdgeRef edge) { selected_elem_id = edge->id(); },
                          [&](Halfedge_Mesh::FaceRef face) {
                              if(!face->is_boundary()) selected_elem_id = face->id();
                          },
                          [&](Halfedge_Mesh::HalfedgeRef halfedge) {
                              if(!halfedge->is_boundary()) selected_elem_id = halfedge->id();
                          }},
               elem);
}

std::tuple<GL::Mesh&, GL::Instances&, GL::Instances&, GL::Instances&> Model::shapes() {
    return {face_mesh, spheres, cylinders, arrows};
}

std::optional<Halfedge_Mesh::ElementRef> Model::selected_element() {

    if(!my_mesh) return std::nullopt;
    if(my_mesh->render_dirty_flag) rebuild();

    auto entry = id_to_info.find(selected_elem_id);
    if(entry == id_to_info.end()) return std::nullopt;
    return entry->second.ref;
}

void Model::unset_mesh() {
    my_mesh = nullptr;
}

void Model::vertex_viz(Halfedge_Mesh::VertexRef v, float& size, Mat4& transform) {

    // Sphere size ~ 0.05 * min incident edge length
    float min = FLT_MAX;
    float avg = 0.0f;
    size_t d = 0;

    auto he = v->halfedge();
    do {
        float len = he->edge()->length();
        min = std::min(min, len);
        avg += len;
        d++;
        he = he->twin()->next();
    } while(he != v->halfedge());

    avg = avg / d;
    size = clamp(min, avg / 10.0f, avg);

    transform = Mat4{Vec4{size, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, size, 0.0f, 0.0f},
                     Vec4{0.0f, 0.0f, size, 0.0f}, Vec4{v->pos, 1.0f}};
}

void Model::edge_viz(Halfedge_Mesh::EdgeRef e, Mat4& transform) {

    auto v_0 = e->halfedge()->vertex();
    auto v_1 = e->halfedge()->twin()->vertex();
    Vec3 v0 = v_0->pos;
    Vec3 v1 = v_1->pos;

    Vec3 dir = v1 - v0;
    float l = dir.norm();
    dir /= l;

    // Cylinder width; 0.6 * min vertex scale
    float v0s = vert_sizes[v_0->id()], v1s = vert_sizes[v_1->id()];
    float s = 0.5f * std::min(v0s, v1s);

    if(dir.y == 1.0f || dir.y == -1.0f) {
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

    auto v_0 = h->vertex();
    auto v_1 = h->twin()->vertex();
    Vec3 v0 = v_0->pos;
    Vec3 v1 = v_1->pos;

    Vec3 dir = v1 - v0;
    float l = dir.norm();
    dir /= l;
    l *= 0.6f;
    // Same width as edge

    float v0s = vert_sizes[v_0->id()], v1s = vert_sizes[v_1->id()];
    float s = 0.3f * (v0s < v1s ? v0s : v1s);

    // Move to center of edge and towards center of face
    Vec3 offset = (v1 - v0) * 0.2f;
    Vec3 base = h->face()->halfedge()->vertex()->pos;

    if(base == v0) {
        base = h->next()->next()->vertex()->pos;
    } else if(base == v1) {
        Halfedge_Mesh::HalfedgeRef hf = h;
        do {
            hf = hf->next();
        } while(hf->next() != h);
        base = hf->vertex()->pos;
    }

    Vec3 face_n = cross(base - v0, base - v1).unit();
    offset += cross(face_n, dir).unit() * s * 0.2f;

    // Align edge
    if(dir.y == 1.0f || dir.y == -1.0f) {
        l *= sign(dir.y);
        transform = Mat4{Vec4{s, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, l, 0.0f, 0.0f},
                         Vec4{0.0f, 0.0f, s, 0.0f}, Vec4{v0 + offset, 1.0f}};
    } else {
        Vec3 x = cross(dir, Vec3{0.0f, 1.0f, 0.0f});
        Vec3 z = cross(x, dir);
        transform = Mat4{Vec4{x * s, 0.0f}, Vec4{dir * l, 0.0f}, Vec4{z * s, 0.0f},
                         Vec4{v0 + offset, 1.0f}};
    }
}

void Model::face_viz(Halfedge_Mesh::FaceRef face, std::vector<GL::Mesh::Vert>& verts,
                     std::vector<GL::Mesh::Index>& idxs, size_t insert_at) {

    std::vector<GL::Mesh::Vert> face_verts;
    unsigned int id = face->id();

    Halfedge_Mesh::HalfedgeRef h = face->halfedge();
    do {
        Halfedge_Mesh::VertexRef v = h->vertex();
        face_verts.push_back({v->pos, {}, 0});
        h = h->next();
    } while(h != face->halfedge());

    id_to_info[face->id()] = {face, insert_at};

    if(face_verts.size() < 3) return;

    size_t max = insert_at + (face_verts.size() - 2) * 3;
    if(verts.size() < max) verts.resize(max);
    if(idxs.size() < max) idxs.resize(max);

    Vec3 v0 = face_verts[0].pos;
    for(size_t i = 1; i <= face_verts.size() - 2; i++) {
        Vec3 v1 = face_verts[i].pos;
        Vec3 v2 = face_verts[i + 1].pos;
        Vec3 n = cross(v1 - v0, v2 - v0).unit();
        if(my_mesh->flipped()) n = -n;

        idxs[insert_at] = (GL::Mesh::Index)insert_at;
        verts[insert_at++] = {v0, n, id};

        idxs[insert_at] = (GL::Mesh::Index)insert_at;
        verts[insert_at++] = {v1, n, id};

        idxs[insert_at] = (GL::Mesh::Index)insert_at;
        verts[insert_at++] = {v2, n, id};
    }
}

void Model::rebuild() {

    if(!my_mesh) return;
    Halfedge_Mesh& mesh = *my_mesh;

    mesh.render_dirty_flag = false;

    id_to_info.clear();
    vert_sizes.clear();

    std::vector<GL::Mesh::Vert> verts;
    std::vector<GL::Mesh::Index> idxs;

    for(auto f = mesh.faces_begin(); f != mesh.faces_end(); f++) {
        if(!f->is_boundary()) face_viz(f, verts, idxs, verts.size());
    }
    face_mesh.recreate(std::move(verts), std::move(idxs));

    // Create sphere for each vertex
    spheres.clear();
    for(auto v = mesh.vertices_begin(); v != mesh.vertices_end(); v++) {

        float d;
        Mat4 transform;
        vertex_viz(v, d, transform);
        vert_sizes[v->id()] = d;
        id_to_info[v->id()] = {v, spheres.add(transform, v->id())};
    }

    // Create cylinder for each edge
    cylinders.clear();
    for(auto e = mesh.edges_begin(); e != mesh.edges_end(); e++) {

        Mat4 transform;
        edge_viz(e, transform);
        id_to_info[e->id()] = {e, cylinders.add(transform, e->id())};
    }

    // Create arrow for each halfedge
    arrows.clear();
    for(auto h = mesh.halfedges_begin(); h != mesh.halfedges_end(); h++) {

        if(h->is_boundary()) continue;

        Mat4 transform;
        halfedge_viz(h, transform);
        id_to_info[h->id()] = {h, arrows.add(transform, h->id())};
    }

    validate();
}

bool Model::begin_bevel(std::string& err) {

    auto sel = selected_element();
    if(sel.has_value()) {
        if(std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(*sel)) {
            return false;
        }
    }

    my_mesh->copy_to(old_mesh);

    Halfedge_Mesh::FaceRef new_face;
    std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
                              auto res = my_mesh->bevel_vertex(vert);
                              if(res.has_value()) {
                                  new_face = *res;
                                  beveling = Bevel::vert;
                              }
                          },
                          [&](Halfedge_Mesh::EdgeRef edge) {
                              auto res = my_mesh->bevel_edge(edge);
                              if(res.has_value()) {
                                  new_face = *res;
                                  beveling = Bevel::edge;
                              }
                          },
                          [&](Halfedge_Mesh::FaceRef face) {
                              auto res = my_mesh->bevel_face(face);
                              if(res.has_value()) {
                                  new_face = *res;
                                  beveling = Bevel::face;
                              }
                          },
                          [&](auto) {}},
               *sel);

    err = validate();
    if(!err.empty()) {

        *my_mesh = std::move(old_mesh);
        return false;

    } else {

        my_mesh->render_dirty_flag = true;
        set_selected(new_face);

        trans_begin = {};
        auto h = new_face->halfedge();
        trans_begin.center = new_face->center();
        do {
            trans_begin.verts.push_back(h->vertex()->pos);
            h = h->next();
        } while(h != new_face->halfedge());

        return true;
    }
}

bool Model::keydown(Widgets& widgets, SDL_Keysym key, Camera& cam) {

    auto sel = selected_element();
    if(!sel.has_value()) return false;

    if(std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(*sel)) {
        auto h = std::get<Halfedge_Mesh::HalfedgeRef>(*sel);
        if(key.sym == SDLK_n) {
            set_selected(h->next());
            return true;
        } else if(key.sym == SDLK_t) {
            set_selected(h->twin());
            return true;
        } else if(key.sym == SDLK_v) {
            set_selected(h->vertex());
            return true;
        } else if(key.sym == SDLK_e) {
            set_selected(h->edge());
            return true;
        } else if(key.sym == SDLK_f) {
            set_selected(h->face());
            return true;
        }
    }

    switch(key.sym) {
    case SDLK_b: widgets.active = Widget_Type::bevel; return true;
    case SDLK_c: zoom_to(*sel, cam); return true;
    case SDLK_h: {
        std::visit(
            overloaded{[&](Halfedge_Mesh::VertexRef vert) { set_selected(vert->halfedge()); },
                       [&](Halfedge_Mesh::EdgeRef edge) { set_selected(edge->halfedge()); },
                       [&](Halfedge_Mesh::FaceRef face) { set_selected(face->halfedge()); },
                       [&](auto) {}},
            *sel);
    }
        return true;
    case SDLK_f: {
        cam.look_at(Vec3{}, -cam.front() * cam.dist());
    }
        return true;
    }

    return false;
}

template<typename T>
std::string Model::update_mesh(Undo& undo, Scene_Object& obj, Halfedge_Mesh&& before,
                               Halfedge_Mesh::ElementRef ref, T&& op) {

    unsigned int id = Halfedge_Mesh::id_of(ref);
    std::optional<Halfedge_Mesh::ElementRef> new_ref = op(*my_mesh, ref);
    if(!new_ref.has_value()) return {};

    auto err = validate();
    if(!err.empty()) {
        obj.take_mesh(std::move(before));
    } else {
        my_mesh->render_dirty_flag = true;
        obj.set_mesh_dirty();
        set_selected(*new_ref);
        undo.update_mesh(obj.id(), std::move(before), id, std::move(op));
    }

    return err;
}

template<typename T>
std::string Model::update_mesh_global(Undo& undo, Scene_Object& obj, Halfedge_Mesh&& before,
                                      T&& op) {

    bool suc = op(*my_mesh);
    if(!suc) return {};

    auto err = validate();
    if(!err.empty()) {
        obj.take_mesh(std::move(before));
    } else {
        my_mesh->render_dirty_flag = true;
        obj.set_mesh_dirty();
        selected_elem_id = 0;
        hovered_elem_id = 0;
        undo.update_mesh_full(obj.id(), std::move(before));
    }
    return err;
}

std::string Model::validate() {

    auto valid = my_mesh->validate();
    if(valid.has_value()) {
        auto& msg = valid.value();
        err_id = Halfedge_Mesh::id_of(msg.first);
        err_msg = msg.second;
        return msg.second;
    }

    auto warn = my_mesh->warnings();
    if(warn.has_value()) {
        auto& msg = warn.value();
        warn_id = Halfedge_Mesh::id_of(msg.first);
        warn_msg = msg.second;
    } else {
        warn_id = 0;
        warn_msg = {};
    }

    return {};
}

void Model::zoom_to(Halfedge_Mesh::ElementRef ref, Camera& cam) {

    float d = cam.dist();
    Vec3 center = Halfedge_Mesh::center_of(ref);
    Vec3 pos = center + my_mesh->normal_of(ref) * d;
    cam.look_at(center, pos);
}

std::optional<std::reference_wrapper<Scene_Object>> Model::set_my_obj(Scene_Maybe obj_opt) {

    if(!obj_opt.has_value()) {
        my_mesh = nullptr;
        return std::nullopt;
    }

    Scene_Item& item = obj_opt.value();
    if(!item.is<Scene_Object>()) {
        my_mesh = nullptr;
        return std::nullopt;
    }

    Scene_Object& obj = item.get<Scene_Object>();
    if(obj.opt.shape_type != PT::Shape_Type::none) {
        my_mesh = nullptr;
        return std::nullopt;
    }

    if(!obj.is_editable()) {
        my_mesh = nullptr;
        return std::nullopt;
    }

    Halfedge_Mesh* old = my_mesh;
    my_mesh = &obj.get_mesh();

    if(old != my_mesh) {
        selected_elem_id = 0;
        hovered_elem_id = 0;
        err_id = 0;
        warn_id = 0;
        rebuild();
    } else if(old->render_dirty_flag) {
        rebuild();
    }
    return obj;
}

std::string Model::UIsidebar(Undo& undo, Widgets& widgets, Scene_Maybe obj_opt, Camera& camera) {

    if(ImGui::CollapsingHeader("Edit Colors")) {
        ImGui::ColorEdit3("Face", f_col.data);
        ImGui::ColorEdit3("Vertex", v_col.data);
        ImGui::ColorEdit3("Edge", e_col.data);
        ImGui::ColorEdit3("Halfedge", he_col.data);
    }

    auto opt = set_my_obj(obj_opt);
    if(!opt.has_value()) return {};
    Scene_Object& obj = opt.value();

    Halfedge_Mesh& mesh = *my_mesh;
    Halfedge_Mesh before;

    ImGui::Separator();
    ImGui::Text("Global Operations");
    if(ImGui::Button("Linear")) {
        mesh.copy_to(before);
        return update_mesh_global(undo, obj, std::move(before),
                                  [](Halfedge_Mesh& m) { return m.subdivide(SubD::linear); });
    }
    if(Manager::wrap_button("Catmull-Clark")) {
        mesh.copy_to(before);
        return update_mesh_global(undo, obj, std::move(before),
                                  [](Halfedge_Mesh& m) { return m.subdivide(SubD::catmullclark); });
    }
    if(Manager::wrap_button("Loop")) {
        mesh.copy_to(before);
        return update_mesh_global(undo, obj, std::move(before),
                                  [](Halfedge_Mesh& m) { return m.subdivide(SubD::loop); });
    }
    if(ImGui::Button("Triangulate")) {
        mesh.copy_to(before);
        return update_mesh_global(undo, obj, std::move(before), [](Halfedge_Mesh& m) {
            m.triangulate();
            return true;
        });
    }
    if(Manager::wrap_button("Remesh")) {
        mesh.copy_to(before);
        return update_mesh_global(undo, obj, std::move(before),
                                  [](Halfedge_Mesh& m) { return m.isotropic_remesh(); });
    }
    if(Manager::wrap_button("Simplify")) {
        mesh.copy_to(before);
        return update_mesh_global(undo, obj, std::move(before),
                                  [](Halfedge_Mesh& m) { return m.simplify(); });
    }

    {
        auto sel = selected_element();
        if(sel.has_value()) {

            ImGui::Text("Local Operations");
            widgets.action_button(Widget_Type::move, "Move [m]", false);
            widgets.action_button(Widget_Type::rotate, "Rotate [r]");
            widgets.action_button(Widget_Type::scale, "Scale [s]");
            widgets.action_button(Widget_Type::bevel, "Bevel [b]");

            std::string err = std::visit(
                overloaded{
                    [&](Halfedge_Mesh::VertexRef vert) -> std::string {
                        if(ImGui::Button("Erase [del]")) {
                            mesh.copy_to(before);
                            return update_mesh(
                                undo, obj, std::move(before), vert,
                                [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef vert) {
                                    return m.erase_vertex(std::get<Halfedge_Mesh::VertexRef>(vert));
                                });
                        }
                        return {};
                    },
                    [&](Halfedge_Mesh::EdgeRef edge) -> std::string {
                        if(ImGui::Button("Erase [del]")) {
                            mesh.copy_to(before);
                            return update_mesh(
                                undo, obj, std::move(before), edge,
                                [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
                                    return m.erase_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
                                });
                        }
                        if(Manager::wrap_button("Collapse")) {
                            mesh.copy_to(before);
                            return update_mesh(
                                undo, obj, std::move(before), edge,
                                [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
                                    return m.collapse_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
                                });
                        }
                        if(Manager::wrap_button("Flip")) {
                            mesh.copy_to(before);
                            return update_mesh(
                                undo, obj, std::move(before), edge,
                                [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
                                    return m.flip_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
                                });
                        }
                        if(Manager::wrap_button("Split")) {
                            mesh.copy_to(before);
                            return update_mesh(
                                undo, obj, std::move(before), edge,
                                [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
                                    return m.split_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
                                });
                        }
                        return {};
                    },
                    [&](Halfedge_Mesh::FaceRef face) -> std::string {
                        if(ImGui::Button("Collapse")) {
                            mesh.copy_to(before);
                            return update_mesh(
                                undo, obj, std::move(before), face,
                                [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef face) {
                                    return m.collapse_face(std::get<Halfedge_Mesh::FaceRef>(face));
                                });
                        }
                        return {};
                    },
                    [&](auto) -> std::string { return {}; }},
                *sel);

            if(!err.empty()) return err;
        }
    }

    {
        auto sel = selected_element();
        if(sel.has_value()) {
            ImGui::Separator();
            ImGui::Text("Navigation");

            if(ImGui::Button("Center Camera [c]")) {
                zoom_to(*sel, camera);
            }
            std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
                                      if(ImGui::Button("Halfedge [h]")) {
                                          set_selected(vert->halfedge());
                                      }
                                  },
                                  [&](Halfedge_Mesh::EdgeRef edge) {
                                      if(ImGui::Button("Halfedge [h]")) {
                                          set_selected(edge->halfedge());
                                      }
                                  },
                                  [&](Halfedge_Mesh::FaceRef face) {
                                      if(ImGui::Button("Halfedge [h]")) {
                                          set_selected(face->halfedge());
                                      }
                                  },
                                  [&](Halfedge_Mesh::HalfedgeRef halfedge) {
                                      if(ImGui::Button("Vertex [v]")) {
                                          set_selected(halfedge->vertex());
                                      }
                                      if(Manager::wrap_button("Edge [e]")) {
                                          set_selected(halfedge->edge());
                                      }
                                      if(Manager::wrap_button("Face [f]")) {
                                          set_selected(halfedge->face());
                                      }
                                      if(Manager::wrap_button("Twin [t]")) {
                                          set_selected(halfedge->twin());
                                      }
                                      if(Manager::wrap_button("Next [n]")) {
                                          set_selected(halfedge->next());
                                      }
                                  }},
                       *sel);
        }
    }

    {
        auto sel = selected_element();
        if(sel.has_value()) {
            ImGui::Separator();
            ImGui::Text("ID Info");

            ImGui::Text("Selected: %u", Halfedge_Mesh::id_of(*sel));
            std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
                                      ImGui::Text("Halfedge: %u", vert->halfedge()->id());
                                  },
                                  [&](Halfedge_Mesh::EdgeRef edge) {
                                      ImGui::Text("Halfedge: %u", edge->halfedge()->id());
                                  },
                                  [&](Halfedge_Mesh::FaceRef face) {
                                      ImGui::Text("Halfedge: %u", face->halfedge()->id());
                                  },
                                  [&](Halfedge_Mesh::HalfedgeRef halfedge) {
                                      ImGui::Text("Vertex: %u", halfedge->vertex()->id());
                                      ImGui::Text("Edge: %u", halfedge->edge()->id());
                                      ImGui::Text("Face: %u", halfedge->face()->id());
                                      ImGui::Text("Twin: %u", halfedge->twin()->id());
                                      ImGui::Text("Next: %u", halfedge->next()->id());
                                  }},
                       *sel);
        }
    }

    if(!err_msg.empty()) {
        ImGui::Separator();
        Vec3 red = Gui::Color::red;
        ImGui::TextColored(ImVec4{red.x, red.y, red.z, 1.0f}, "Error");
        ImGui::TextWrapped("%s", err_msg.c_str());
        ImGui::TextWrapped("(Your operation resulted in an invalid mesh.)");
        if(ImGui::Button("Select Error")) {
            selected_elem_id = err_id;
        }
        if(Manager::wrap_button("Clear")) {
            err_id = 0;
            err_msg = {};
        }
    }
    if(!warn_msg.empty()) {
        ImGui::Separator();
        Vec3 red = Gui::Color::red;
        ImGui::TextColored(ImVec4{red.x, red.y, red.z, 1.0f}, "Warning");
        ImGui::TextWrapped("%s", warn_msg.c_str());
        ImGui::TextWrapped("(Your mesh is still manifold, but may cause issues when exported.)");
        if(ImGui::Button("Select Next")) {
            selected_elem_id = warn_id;
        }
    }

    ImGui::Separator();
    return {};
}

void Model::erase_selected(Undo& undo, Scene_Maybe obj_opt) {

    auto opt = set_my_obj(obj_opt);
    if(!opt.has_value()) return;
    Scene_Object& obj = opt.value();

    auto sel_ = selected_element();
    if(!sel_.has_value()) return;

    Halfedge_Mesh::ElementRef sel = sel_.value();
    Halfedge_Mesh& mesh = *my_mesh;

    Halfedge_Mesh before;
    mesh.copy_to(before);

    std::visit(overloaded{[&](Halfedge_Mesh::VertexRef vert) {
                              return update_mesh(
                                  undo, obj, std::move(before), vert,
                                  [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef vert) {
                                      return m.erase_vertex(
                                          std::get<Halfedge_Mesh::VertexRef>(vert));
                                  });
                          },
                          [&](Halfedge_Mesh::EdgeRef edge) {
                              return update_mesh(
                                  undo, obj, std::move(before), edge,
                                  [](Halfedge_Mesh& m, Halfedge_Mesh::ElementRef edge) {
                                      return m.erase_edge(std::get<Halfedge_Mesh::EdgeRef>(edge));
                                  });
                          },
                          [](auto) { return std::string{}; }},
               sel);
}

void Model::clear_select() {
    selected_elem_id = 0;
}

void Model::render(Scene_Maybe obj_opt, Widgets& widgets, Camera& cam) {

    auto obj = set_my_obj(obj_opt);
    if(!obj.has_value()) return;

    Mat4 view = cam.get_view();

    Renderer::HalfedgeOpt opts(*this);
    opts.modelview = view;
    opts.v_color = v_col;
    opts.f_color = f_col;
    opts.e_color = e_col;
    opts.he_color = he_col;
    opts.err_color = err_col;
    opts.err_id = err_id;
    Renderer::get().halfedge_editor(opts);

    auto elem = selected_element();
    if(elem.has_value()) {
        auto e = *elem;
        Vec3 pos = Halfedge_Mesh::center_of(e);
        if(!std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(e)) {
            float scl = std::min((cam.pos() - pos).norm() / 5.5f, 10.0f);
            widgets.render(view, pos, scl);
        }
    }
}

std::string Model::end_transform(Widgets& widgets, Undo& undo, Scene_Object& obj) {

    obj.set_mesh_dirty();
    my_mesh->render_dirty_flag = true;

    auto err = validate();
    if(!err.empty()) {
        obj.take_mesh(std::move(old_mesh));
    } else {
        undo.update_mesh_full(obj.id(), std::move(old_mesh));
    }
    return err;
}

Vec3 Model::selected_pos() {
    auto elem = selected_element();
    assert(elem.has_value());
    return Halfedge_Mesh::center_of(*elem);
}

std::string Model::select(Widgets& widgets, Scene_ID click, Vec3 cam, Vec2 spos, Vec3 dir) {

    if(click && widgets.active == Widget_Type::bevel && click == selected_elem_id) {

        std::string err;
        if(!begin_bevel(err)) {
            widgets.end_drag();
            return err;
        } else {
            widgets.start_drag(Halfedge_Mesh::center_of(selected_element().value()), cam, spos,
                               dir);
            apply_transform(widgets);
        }

    } else if(!widgets.is_dragging() && click >= n_Widget_IDs) {
        selected_elem_id = (unsigned int)click;
    }

    if(widgets.want_drag()) {
        auto e = selected_element();
        if(e.has_value() && !std::holds_alternative<Halfedge_Mesh::HalfedgeRef>(*e)) {
            widgets.start_drag(Halfedge_Mesh::center_of(*e), cam, spos, dir);
            if(widgets.active != Widget_Type::bevel) {
                begin_transform();
            }
        }
    }
    return {};
}

unsigned int Model::select_id() const {
    return selected_elem_id;
}

unsigned int Model::hover_id() const {
    return hovered_elem_id;
}

void Model::hover(unsigned int id) {
    hovered_elem_id = id;
}

} // namespace Gui
