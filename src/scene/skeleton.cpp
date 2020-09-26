
#include "skeleton.h"
#include "../gui/manager.h"
#include "renderer.h"

bool Joint::is_root() const { return !parent; }

void Joint::for_joints(std::function<void(Joint *)> func) {
    func(this);
    for (Joint *j : children)
        j->for_joints(func);
}

Skeleton::Skeleton() {
    root_id = Gui::n_Widget_IDs;
    next_id = Gui::n_Widget_IDs + 1;
}

Skeleton::Skeleton(unsigned int obj_id) {
    root_id = obj_id + 1;
    next_id = root_id + 1;
}

Skeleton::~Skeleton() {
    for (Joint *j : roots)
        delete j;
    for (Joint *j : erased)
        delete j;
}

bool Skeleton::set_time(float time) {
    bool ret = false;
    for_joints([&ret, time](Joint *j) {
        if (j->anim.any()) {
            j->pose = j->anim.at(time).to_euler();
            ret = true;
        }
    });
    return ret;
}

void Skeleton::for_joints(std::function<void(Joint *)> func) {
    for (Joint *r : roots)
        r->for_joints(func);
}

Joint *Skeleton::add_root(Vec3 extent) {
    Joint *j = new Joint(next_id++, nullptr, extent);
    for (float f : keys()) {
        j->anim.set(f, Quat{});
    }
    roots.insert(j);
    return j;
}

void Skeleton::crop(float t) {
    for_joints([t](Joint *j) { j->anim.crop(t); });
}

Joint *Skeleton::get_joint(unsigned int id) {
    Joint *j = nullptr;
    for_joints([&](Joint *jt) {
        if (jt->_id == id)
            j = jt;
    });
    return j;
}

void Skeleton::render(const Mat4 &view, Joint *select, bool root, bool posed, unsigned int offset) {

    Renderer &R = Renderer::get();

    Mat4 V = view * Mat4::translate(base_pos);
    for_joints([&](Joint *j) {
        Renderer::MeshOpt opt;
        opt.modelview =
            V * (posed ? j->joint_to_posed() : j->joint_to_bind()) * Mat4::rotate_to(j->extent);
        opt.id = j->_id + offset;
        opt.alpha = 0.8f;
        opt.color = Gui::Color::hover;
        R.capsule(opt, j->extent.norm(), j->radius);
    });

    if (select) {
        R.begin_outline();

        Mat4 model = Mat4::translate(base_pos) *
                     (posed ? select->joint_to_posed() : select->joint_to_bind()) *
                     Mat4::rotate_to(select->extent);

        Renderer::MeshOpt opt;
        opt.modelview = view;
        opt.id = select->_id + offset;
        opt.depth_only = true;

        BBox box;
        R.capsule(opt, model, select->extent.norm(), select->radius, box);
        R.end_outline(view, box);
    }
    R.reset_depth();

    Renderer::MeshOpt opt;
    opt.id = root_id + offset;
    opt.modelview = V * Mat4::scale(Vec3{0.1f});
    opt.color = root ? Gui::Color::outline : Gui::Color::hover;
    R.sphere(opt);

    for_joints([&](Joint *j) {
        Renderer::MeshOpt opt;
        opt.modelview = V * (posed ? j->joint_to_posed() : j->joint_to_bind()) *
                        Mat4::translate(j->extent) * Mat4::scale(Vec3{j->radius * 0.25f});
        opt.id = j->_id + offset;
        opt.color = select == j ? Gui::Color::outline : Gui::Color::hover;
        R.sphere(opt);
    });
}

void Skeleton::outline(const Mat4 &view, Joint *select, bool root, bool posed, BBox &box,
                       unsigned int offset) {

    Renderer &R = Renderer::get();
    Mat4 base_t = Mat4::translate(base_pos);

    for_joints([&](Joint *j) {
        Mat4 model = base_t * (posed ? j->joint_to_posed() : j->joint_to_bind()) *
                     Mat4::rotate_to(j->extent);
        Renderer::MeshOpt opt;
        opt.modelview = view;
        opt.id = j->_id + offset;
        opt.depth_only = true;

        R.capsule(opt, model, j->extent.norm(), j->radius, box);
    });
}

bool Skeleton::is_root_id(unsigned int id) { return id == root_id; }

Joint *Skeleton::parent(Joint *j) { return j->parent; }

bool Skeleton::has_bones() const { return !roots.empty(); }

unsigned int Skeleton::n_bones() {
    unsigned int n = 0;
    for_joints([&n](Joint *) { n++; });
    return n;
}

Vec3 &Skeleton::base() { return base_pos; }

Joint *Skeleton::add_child(Joint *j, Vec3 e) {
    Joint *c = new Joint(next_id++, j, e);
    for (float f : keys()) {
        c->anim.set(f, Quat{});
    }
    j->children.insert(c);
    return c;
}

void Skeleton::restore(Joint *j) {
    if (j->parent) {
        j->parent->children.insert(j);
    } else {
        roots.insert(j);
    }
    erased.erase(j);
}

void Skeleton::erase(Joint *j) {
    if (j->parent) {
        j->parent->children.erase(j);
    } else {
        roots.erase(j);
    }
    erased.insert(j);
}

Vec3 Skeleton::posed_base_of(Joint *j) { return j->is_root() ? base() : posed_end_of(parent(j)); }

Vec3 Skeleton::base_of(Joint *j) { return j->is_root() ? base() : end_of(parent(j)); }

void Skeleton::set(float t) {
    for_joints([t](Joint *j) { j->anim.set(t, Quat::euler(j->pose)); });
}

void Skeleton::erase(float t) {
    for_joints([t](Joint *j) { j->anim.erase(t); });
}

bool Skeleton::has_keyframes() {
    bool frame = false;
    for_joints([&frame](Joint *j) { frame = frame || j->anim.any(); });
    return frame;
}

std::unordered_map<unsigned int, Vec3> Skeleton::now() {
    std::unordered_map<unsigned int, Vec3> ret;
    for_joints([&ret](Joint *j) { ret[j->_id] = j->pose; });
    return ret;
}

std::set<float> Skeleton::keys() {
    std::set<float> ret;
    for_joints([&ret](Joint *j) {
        std::set<float> k = j->anim.keys();
        ret.insert(k.begin(), k.end());
    });
    return ret;
}

std::unordered_map<unsigned int, Vec3> Skeleton::at(float t) {
    std::unordered_map<unsigned int, Vec3> ret;
    for_joints([&ret, t](Joint *j) { ret[j->_id] = j->anim.at(t).to_euler(); });
    return ret;
}

void Skeleton::set(float t, const std::unordered_map<unsigned int, Vec3> &data) {
    for_joints([&data, t](Joint *j) { j->anim.set(t, Quat::euler(data.at(j->_id))); });
}
