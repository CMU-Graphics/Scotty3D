
#include "../scene/skeleton.h"

Vec3 closest_on_line_segment(Vec3 start, Vec3 end, Vec3 point) {

    // TODO(Animation): Task 3

    // Return the closest point to 'point' on the line segment from start to end
    return Vec3{};
}

Mat4 Joint::joint_to_bind() const {

    // TODO(Animation): Task 2

    // Return a matrix transforming points in the space of this joint
    // to points in mesh space in bind position.

    // Bind position implies that all joints have pose = Vec3{0.0f}

    // You will need to traverse the joint heirarchy. This should
    // not take into account Skeleton::base_pos
    return Mat4::I;
}

Mat4 Joint::joint_to_posed() const {

    // TODO(Animation): Task 2

    // Return a matrix transforming points in the space of this joint
    // to points in mesh space, taking into account joint poses.

    // You will need to traverse the joint heirarchy. This should
    // not take into account Skeleton::base_pos
    return Mat4::I;
}

Vec3 Skeleton::end_of(Joint *j) {

    // TODO(Animation): Task 2

    // Return the position of the endpoint of joint j in mesh space in bind position.
    // This should take into account Skeleton::base_pos.
    return Vec3{};
}

Vec3 Skeleton::posed_end_of(Joint *j) {

    // TODO(Animation): Task 2

    // Return the position of the endpoint of joint j in mesh space with poses.
    // This should take into account Skeleton::base_pos.
    return Vec3{};
}

Mat4 Skeleton::joint_to_bind(const Joint *j) const {

    // TODO(Animation): Task 2

    // Return a matrix transforming points in joint j's space to mesh space in
    // bind position. This should take into account Skeleton::base_pos.
    return Mat4::I;
}

Mat4 Skeleton::joint_to_posed(const Joint *j) const {

    // TODO(Animation): Task 2

    // Return a matrix transforming points in joint j's space to mesh space with
    // poses. This should take into account Skeleton::base_pos.
    return Mat4::I;
}

void Skeleton::find_joints(const GL::Mesh &mesh,
                           std::unordered_map<unsigned int, std::vector<Joint *>> &map) {

    // TODO(Animation): Task 3

    // Construct a mapping from vertex indices in 'mesh' to lists of joints in this skeleton
    // that should effect the vertex. A joint should effect a vertex if it is within Joint::radius
    // distance of the bone segment in bind position.

    const std::vector<GL::Mesh::Vert> &verts = mesh.verts();

    for_joints([&](Joint *j) {
        // What vertices does joint j effect?
    });

    // For each i in [0,verts.size()), map[i] should contain the list of joints that effect vertex i
    (void)verts;
}

void Skeleton::skin(const GL::Mesh &input, GL::Mesh &output,
                    const std::unordered_map<unsigned int, std::vector<Joint *>> &map) {

    // TODO(Animation): Task 3

    // Apply bone poses & weights to the vertices of the input (bind position) mesh
    // and store the result in the output mesh. See the task description for details.
    // map was computed by find_joints, hence gives a mapping from vertex index to
    // the list of bones the vertex should be effected by.

    // Currently, this just copies the input to the output without modification.

    std::vector<GL::Mesh::Vert> verts = input.verts();
    for (size_t i = 0; i < verts.size(); i++) {

        // Skin vertex i.
    }

    std::vector<GL::Mesh::Index> idxs = input.indices();
    output.recreate(std::move(verts), std::move(idxs));
}
