
#include "skeleton.h"
#include "../gui/manager.h"
#include "renderer.h"

Joint::Joint(unsigned int id) : _id(id) {
}

Joint::Joint(unsigned int id, Joint* parent, Vec3 extent)
    : _id(id), parent(parent), extent(extent) {
}

Joint::~Joint() {
    for(Joint* j : children) delete j;
}

unsigned int Joint::id() const {
    return _id;
}

bool Joint::is_root() const {
    return !parent;
}

void Joint::for_joints(std::function<void(Joint*)> func) {
    func(this);
    for(Joint* j : children) j->for_joints(func);
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
    for(Joint* j : roots) delete j;
    for(auto& e : erased) delete e.first;
    for(IK_Handle* h : handles) delete h;
    for(IK_Handle* h : erased_handles) delete h;
}

bool Skeleton::set_time(float time) {
    bool ret = false;
    for_joints([&ret, time](Joint* j) {
        if(j->anim.any()) {
            j->pose = j->anim.at(time).to_euler();
            ret = true;
        }
    });
    for(IK_Handle* h : handles) {
        if(h->anim.any()) {
            auto [t, e] = h->anim.at(time);
            h->target = t;
            h->enabled = e;
        }
    }
    return ret;
}

void Skeleton::for_joints(std::function<void(Joint*)> func) {
    for(Joint* r : roots) r->for_joints(func);
}

void Skeleton::for_handles(std::function<void(Skeleton::IK_Handle*)> func) {
    for(IK_Handle* h : handles) func(h);
}

Joint* Skeleton::add_root(Vec3 extent) {
    Joint* j = new Joint(next_id++, nullptr, extent);
    for(float f : keys()) {
        j->anim.set(f, Quat{});
    }
    roots.insert(j);
    return j;
}

void Skeleton::crop(float t) {
    for_joints([t](Joint* j) { j->anim.crop(t); });
    for(IK_Handle* h : handles) {
        h->anim.crop(t);
    }
}

Skeleton::IK_Handle* Skeleton::get_handle(unsigned int id) {
    IK_Handle* j = nullptr;
    for(IK_Handle* h : handles) {
        if(h->_id == id) j = h;
    }
    return j;
}

Joint* Skeleton::get_joint(unsigned int id) {
    Joint* j = nullptr;
    for_joints([&](Joint* jt) {
        if(jt->_id == id) j = jt;
    });
    return j;
}

void Skeleton::render(const Mat4& view, Joint* jselect, IK_Handle* hselect, bool root, bool posed,
                      unsigned int offset) {

    Renderer& R = Renderer::get();

    Mat4 V = view * Mat4::translate(base_pos);
    for_joints([&](Joint* j) {
        Renderer::MeshOpt opt;
        opt.modelview =
            V * (posed ? j->joint_to_posed() : j->joint_to_bind()) * Mat4::rotate_to(j->extent);
        opt.id = j->_id + offset;
        opt.alpha = 0.8f;
        opt.color = Gui::Color::hover;
        R.capsule(opt, j->extent.norm(), j->radius);
    });

    if(jselect) {
        R.begin_outline();

        Mat4 model = Mat4::translate(base_pos) *
                     (posed ? jselect->joint_to_posed() : jselect->joint_to_bind()) *
                     Mat4::rotate_to(jselect->extent);

        Renderer::MeshOpt opt;
        opt.modelview = view;
        opt.id = jselect->_id + offset;
        opt.depth_only = true;

        BBox box;
        R.capsule(opt, model, jselect->extent.norm(), jselect->radius, box);
        R.end_outline(view, box);
    }
    R.reset_depth();

    {
        Renderer::MeshOpt opt;
        opt.id = root_id + offset;
        opt.modelview = V * Mat4::scale(Vec3{0.1f});
        opt.color = root ? Gui::Color::outline : Gui::Color::hover;
        R.sphere(opt);
    }

    for_joints([&](Joint* j) {
        Renderer::MeshOpt opt;
        opt.modelview = V * (posed ? j->joint_to_posed() : j->joint_to_bind()) *
                        Mat4::translate(j->extent) * Mat4::scale(Vec3{j->radius * 0.25f});
        opt.id = j->_id + offset;
        opt.color = jselect == j ? Gui::Color::outline : Gui::Color::hover;
        R.sphere(opt);
    });

    GL::Lines ik_lines;
    for(IK_Handle* h : handles) {
        Renderer::MeshOpt opt;
        opt.modelview = V * Mat4::translate(h->target) * Mat4::scale(Vec3{h->joint->radius * 0.3f});
        opt.id = h->_id + offset;
        opt.color = hselect == h ? Gui::Color::outline : Gui::Color::hoverg;
        Vec3 j_world = posed ? posed_end_of(h->joint) : end_of(h->joint);
        ik_lines.add(h->target, j_world - base_pos,
                     h->enabled ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f));
        R.sphere(opt);
    }
    R.lines(ik_lines, V);
}

void Skeleton::outline(const Mat4& view, const Mat4& model, bool root, bool posed, BBox& box,
                       unsigned int offset) {

    Renderer& R = Renderer::get();
    Mat4 base_t = Mat4::translate(base_pos);

    for_joints([&](Joint* j) {
        Mat4 M = model * base_t * (posed ? j->joint_to_posed() : j->joint_to_bind()) *
                 Mat4::rotate_to(j->extent);
        Renderer::MeshOpt opt;
        opt.modelview = view;
        opt.id = j->_id + offset;
        opt.depth_only = true;

        R.capsule(opt, M, j->extent.norm(), j->radius, box);
    });
}

bool Skeleton::is_root_id(unsigned int id) {
    return id == root_id;
}

Joint* Skeleton::parent(Joint* j) {
    return j->parent;
}

bool Skeleton::has_bones() const {
    return !roots.empty();
}

unsigned int Skeleton::n_bones() {
    unsigned int n = 0;
    for_joints([&n](Joint*) { n++; });
    return n;
}

unsigned int Skeleton::n_handles() {
    return (unsigned int)handles.size();
}

Vec3& Skeleton::base() {
    return base_pos;
}

Joint* Skeleton::add_child(Joint* j, Vec3 e) {
    Joint* c = new Joint(next_id++, j, e);
    for(float f : keys()) {
        c->anim.set(f, Quat{});
    }
    j->children.insert(c);
    return c;
}

void Skeleton::restore(Joint* j) {
    if(j->parent) {
        j->parent->children.insert(j);
    } else {
        roots.insert(j);
    }

    auto entry = erased.find(j);
    assert(entry != erased.end());

    for(IK_Handle* handle : entry->second) {
        restore(handle);
    }
    erased.erase(j);
}

void Skeleton::erase(Joint* j) {
    if(j->parent) {
        j->parent->children.erase(j);
    } else {
        roots.erase(j);
    }
    std::vector<IK_Handle*> herase;
    for(IK_Handle* h : handles) {
        if(h->joint == j) {
            herase.push_back(h);
        }
    }
    for(IK_Handle* h : herase) {
        erase(h);
    }
    erased.insert({j, std::move(herase)});
}

Vec3 Skeleton::posed_base_of(Joint* j) {
    return j->is_root() ? base() : posed_end_of(parent(j));
}

Vec3 Skeleton::base_of(Joint* j) {
    return j->is_root() ? base() : end_of(parent(j));
}

void Skeleton::set(float t) {
    for_joints([t](Joint* j) { j->anim.set(t, Quat::euler(j->pose)); });
    for(IK_Handle* h : handles) {
        h->anim.set(t, h->target, h->enabled);
    }
}

bool Skeleton::has(float t) {
    bool had = false;
    for_joints([t, &had](Joint* j) { had = had || j->anim.has(t); });
    for(IK_Handle* h : handles) {
        had = had || h->anim.has(t);
    }
    return had;
}

void Skeleton::erase(float t) {
    for_joints([t](Joint* j) { j->anim.erase(t); });
    for(IK_Handle* h : handles) {
        h->anim.erase(t);
    }
}

bool Skeleton::has_keyframes() {
    bool frame = false;
    for_joints([&frame](Joint* j) { frame = frame || j->anim.any(); });
    for(IK_Handle* h : handles) {
        frame = frame || h->anim.any();
    }
    return frame;
}

Skeleton::VSave Skeleton::now() {
    VSave ret;
    for_joints([&ret](Joint* j) { ret[j->_id] = {j->pose}; });
    for(IK_Handle* h : handles) {
        ret[h->_id] = {std::make_pair(h->target, h->enabled)};
    }
    return ret;
}

Skeleton::SSave Skeleton::splines() {
    SSave ret;
    for_joints([&ret](Joint* j) { ret[j->_id] = {j->anim}; });
    for(IK_Handle* h : handles) {
        ret[h->_id] = {h->anim};
    }
    return ret;
}

void Skeleton::restore_splines(const Skeleton::SSave& data) {
    for_joints([&data](Joint* j) { j->anim = std::get<Spline<Quat>>(data.at(j->_id)); });
    for(IK_Handle* h : handles) {
        h->anim = std::get<Splines<Vec3, bool>>(data.at(h->_id));
    }
}

std::set<float> Skeleton::keys() {
    std::set<float> ret;
    for_joints([&ret](Joint* j) {
        std::set<float> k = j->anim.keys();
        ret.insert(k.begin(), k.end());
    });
    for(IK_Handle* h : handles) {
        std::set<float> k = h->anim.keys();
        ret.insert(k.begin(), k.end());
    }
    return ret;
}

Skeleton::VSave Skeleton::at(float t) {
    VSave ret;
    for_joints([&ret, t](Joint* j) { ret[j->_id] = {j->anim.at(t).to_euler()}; });
    for(IK_Handle* h : handles) {
        auto [tr, e] = h->anim.at(t);
        ret[h->_id] = {std::make_pair(tr, e)};
    }
    return ret;
}

void Skeleton::set(float t, const Skeleton::VSave& data) {
    for_joints(
        [&data, t](Joint* j) { j->anim.set(t, Quat::euler(std::get<Vec3>(data.at(j->_id)))); });
    for(IK_Handle* h : handles) {
        auto [tr, e] = std::get<std::pair<Vec3, bool>>(data.at(h->_id));
        h->anim.set(t, tr, e);
    }
}

void Skeleton::restore(IK_Handle* h) {
    handles.insert(h);
    erased_handles.erase(h);
}

void Skeleton::erase(IK_Handle* h) {
    handles.erase(h);
    erased_handles.insert(h);
}

Skeleton::IK_Handle* Skeleton::add_handle(Vec3 pos, Joint* j) {
    IK_Handle* handle = new IK_Handle{pos - base_pos, j, {}, false, next_id++};
    handles.insert(handle);
    return handle;
}

bool Skeleton::do_ik() {
    std::vector<IK_Handle*> enabled;
    for(IK_Handle* h : handles) {
        if(h->enabled) {
            enabled.push_back(h);
        }
    }
    if(enabled.empty()) return false;
    step_ik(std::move(enabled));
    return true;
}
