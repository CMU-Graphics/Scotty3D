
#pragma once

#include "../geometry/halfedge.h"
#include "../platform/gl.h"
#include "../rays/shapes.h"

#include "material.h"
#include "pose.h"
#include "skeleton.h"

using Scene_ID = unsigned int;

class Scene_Object {
public:
    Scene_Object() = default;
    Scene_Object(Scene_ID id, Pose pose, GL::Mesh&& mesh, std::string n = {});
    Scene_Object(Scene_ID id, Pose pose, Halfedge_Mesh&& mesh, std::string n = {});
    Scene_Object(const Scene_Object& src) = delete;
    Scene_Object(Scene_Object&& src) = default;
    ~Scene_Object() = default;

    void operator=(const Scene_Object& src) = delete;
    Scene_Object& operator=(Scene_Object&& src) = default;

    Scene_ID id() const;
    void sync_mesh();
    void sync_anim_mesh();
    void set_time(float time);

    const GL::Mesh& mesh();
    const GL::Mesh& posed_mesh();

    void render(const Mat4& view, bool solid = false, bool depth_only = false, bool posed = true,
                bool anim = true);

    Halfedge_Mesh& get_mesh();
    const Halfedge_Mesh& get_mesh() const;
    void copy_mesh(Halfedge_Mesh& out);
    void take_mesh(Halfedge_Mesh&& in);
    void set_mesh(Halfedge_Mesh& in);
    Halfedge_Mesh::ElementRef set_mesh(Halfedge_Mesh& in, unsigned int eid);

    BBox bbox();
    bool is_editable() const;
    bool is_shape() const;
    void try_make_editable(PT::Shape_Type prev = PT::Shape_Type::none);
    void flip_normals();

    void set_mesh_dirty();
    void set_skel_dirty();
    void set_pose_dirty();

    static const inline int max_name_len = 256;
    struct Options {
        char name[max_name_len] = {};
        bool wireframe = false;
        bool smooth_normals = false;
        PT::Shape_Type shape_type = PT::Shape_Type::none;
        PT::Shape shape;
    };

    Options opt;
    Pose pose;
    Anim_Pose anim;
    Skeleton armature;
    Material material;

    mutable bool rig_dirty = false;

private:
    Scene_ID _id = 0;
    Halfedge_Mesh halfedge;

    mutable GL::Mesh _mesh, _anim_mesh;
    mutable std::unordered_map<unsigned int, std::vector<Joint*>> vertex_joints;
    mutable bool editable = true;
    mutable bool mesh_dirty = false;
    mutable bool skel_dirty = false, pose_dirty = false;
};

bool operator!=(const Scene_Object::Options& l, const Scene_Object::Options& r);
