
#include <sstream>

#include "object.h"
#include "renderer.h"

#include "../geometry/util.h"
#include "../gui/render.h"

Scene_Object::Scene_Object(Scene_ID id, Pose p, GL::Mesh&& m, std::string n)
    : pose(p), _id(id), armature(id), _mesh(std::move(m)) {

    set_skel_dirty();
    editable = false;

    if(n.size()) {
        snprintf(opt.name, max_name_len, "%s", n.c_str());
    } else {
        snprintf(opt.name, max_name_len, "Object %d", id);
    }
}

Scene_Object::Scene_Object(Scene_ID id, Pose p, Halfedge_Mesh&& m, std::string n)
    : pose(p), _id(id), armature(id), halfedge(std::move(m)), _mesh() {

    set_mesh_dirty();

    if(n.size()) {
        snprintf(opt.name, max_name_len, "%s", n.c_str());
    } else {
        snprintf(opt.name, max_name_len, "Object %d", id);
    }

    sync_anim_mesh();
}

const GL::Mesh& Scene_Object::posed_mesh() {
    sync_anim_mesh();
    if(armature.has_bones()) {
        return _anim_mesh;
    }
    return _mesh;
}

Scene_ID Scene_Object::id() const {
    return _id;
}

const GL::Mesh& Scene_Object::mesh() {
    sync_mesh();
    return _mesh;
}

void Scene_Object::try_make_editable(PT::Shape_Type prev) {

    switch(prev) {
    case PT::Shape_Type::sphere: {
        _mesh = Util::sphere_mesh(opt.shape.get<PT::Sphere>().radius, 2);
    } break;

    case PT::Shape_Type::none:
    case PT::Shape_Type::count: break;
    }

    std::string err = halfedge.from_mesh(_mesh);
    if(err.empty()) {
        editable = true;
        opt.smooth_normals = true;
    }

    mesh_dirty = true;
    skel_dirty = true;
}

bool Scene_Object::is_shape() const {
    return opt.shape_type != PT::Shape_Type::none;
}

void Scene_Object::set_time(float time) {
    if(anim.splines.any()) pose = anim.at(time);
    if(armature.set_time(time)) set_pose_dirty();
    if(material.anim.splines.any()) material.anim.at(time, material.opt);
}

bool Scene_Object::is_editable() const {
    return editable && opt.shape_type == PT::Shape_Type::none;
}

void Scene_Object::copy_mesh(Halfedge_Mesh& out) {
    halfedge.copy_to(out);
}

void Scene_Object::set_mesh(Halfedge_Mesh& in) {
    in.copy_to(halfedge);
    set_mesh_dirty();
}

Halfedge_Mesh::ElementRef Scene_Object::set_mesh(Halfedge_Mesh& in, unsigned int eid) {
    auto e = in.copy_to(halfedge, eid);
    set_mesh_dirty();
    return e;
}

void Scene_Object::take_mesh(Halfedge_Mesh&& in) {
    halfedge = std::move(in);
    set_mesh_dirty();
}

Halfedge_Mesh& Scene_Object::get_mesh() {
    return halfedge;
}

const Halfedge_Mesh& Scene_Object::get_mesh() const {
    return halfedge;
}

void Scene_Object::sync_anim_mesh() {
    sync_mesh();
    if(skel_dirty && armature.has_bones()) {
        vertex_joints.clear();
        armature.find_joints(_mesh, vertex_joints);
    }
    if(pose_dirty && armature.has_bones()) {
        armature.skin(_mesh, _anim_mesh, vertex_joints);
        if(!opt.smooth_normals) {
            auto& verts = _anim_mesh.edit_verts();
            auto& idxs = _anim_mesh.edit_indices();
            for(size_t i = 0; i < idxs.size(); i += 3) {
                Vec3 v0 = verts[idxs[i]].pos;
                Vec3 v1 = verts[idxs[i + 1]].pos;
                Vec3 v2 = verts[idxs[i + 2]].pos;
                Vec3 n = cross(v1 - v0, v2 - v0).unit();
                verts[idxs[i]].norm = n;
                verts[idxs[i + 1]].norm = n;
                verts[idxs[i + 2]].norm = n;
            }
        }
    }
    skel_dirty = pose_dirty = false;
}

void Scene_Object::flip_normals() {
    halfedge.flip();
    mesh_dirty = true;
}

void Scene_Object::sync_mesh() {

    if(editable && mesh_dirty) {
        halfedge.to_mesh(_mesh, !opt.smooth_normals);
        mesh_dirty = false;
    } else if(mesh_dirty && is_shape()) {
        mesh_dirty = false;
    }
}

void Scene_Object::set_pose_dirty() {
    pose_dirty = true;
}

void Scene_Object::set_skel_dirty() {
    skel_dirty = true;
    pose_dirty = true;
}

void Scene_Object::set_mesh_dirty() {
    rig_dirty = true;
    mesh_dirty = true;
    skel_dirty = true;
    pose_dirty = true;
}

BBox Scene_Object::bbox() {

    sync_anim_mesh();

    BBox box;
    if(opt.shape_type == PT::Shape_Type::none) {
        if(armature.has_bones())
            box = _anim_mesh.bbox();
        else
            box = _mesh.bbox();
    } else {
        box = opt.shape.bbox();
    }
    box.transform(pose.transform());
    return box;
}

void Scene_Object::render(const Mat4& view, bool solid, bool depth_only, bool posed, bool do_anim) {

    if(do_anim)
        sync_anim_mesh();
    else
        sync_mesh();

    Renderer::MeshOpt opts;
    opts.id = _id;
    opts.solid_color = solid;
    opts.depth_only = depth_only;
    opts.color = material.layout_color();
    opts.sel_color = material.layout_color();
    opts.modelview = posed ? view * pose.transform() : view;

    switch(opt.shape_type) {
    case PT::Shape_Type::sphere: {
        opts.wireframe = false;
        opts.modelview = opts.modelview * Mat4::scale(Vec3{opt.shape.get<PT::Sphere>().radius});
        Renderer::get().sphere(opts);
    } break;
    case PT::Shape_Type::none: {
        opts.wireframe = opt.wireframe;

        if(do_anim && armature.has_bones()) {
            Renderer::get().mesh(_anim_mesh, opts);
        } else {
            Renderer::get().mesh(_mesh, opts);
        }
    } break;

    case PT::Shape_Type::count: break;
    }
}

bool operator!=(const Scene_Object::Options& l, const Scene_Object::Options& r) {
    return std::string(l.name) != std::string(r.name) || l.shape_type != r.shape_type ||
           l.smooth_normals != r.smooth_normals || l.wireframe != r.wireframe || l.shape != r.shape;
}
