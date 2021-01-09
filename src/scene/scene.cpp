
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <sstream>

#include "../gui/manager.h"
#include "../gui/render.h"
#include "../lib/log.h"

#include "renderer.h"
#include "scene.h"
#include "undo.h"

namespace std {
template<typename T1, typename T2> struct hash<pair<T1, T2>> {
    uint64_t operator()(const pair<T1, T2>& p) const {
        static const hash<T1> h1;
        static const hash<T2> h2;
        return h1(p.first) ^ h2(p.second);
    }
};
} // namespace std

Scene_Item::Scene_Item(Scene_Object&& obj) : data(std::move(obj)) {
}

Scene_Item::Scene_Item(Scene_Light&& light) : data(std::move(light)) {
}

Scene_Item::Scene_Item(Scene_Particles&& particles) : data(std::move(particles)) {
}

Scene_Item::Scene_Item(Scene_Item&& src) : data(std::move(src.data)) {
}

Scene_Item& Scene_Item::operator=(Scene_Item&& src) {
    data = std::move(src.data);
    return *this;
}

void Scene_Item::set_time(float time) {
    return std::visit([time](auto& obj) { obj.set_time(time); }, data);
}

BBox Scene_Item::bbox() {
    return std::visit([](auto& obj) { return obj.bbox(); }, data);
}

void Scene_Item::render(const Mat4& view, bool solid, bool depth_only, bool posed) {
    std::visit(
        overloaded{[&](Scene_Object& obj) { obj.render(view, solid, depth_only, posed); },
                   [&](Scene_Light& light) { light.render(view, depth_only, posed); },
                   [&](Scene_Particles& particles) { particles.render(view, depth_only, posed); }},
        data);
}

Scene_ID Scene_Item::id() const {
    return std::visit([](auto& obj) { return obj.id(); }, data);
}

Anim_Pose& Scene_Item::animation() {

    Scene_Object* o = std::get_if<Scene_Object>(&data);
    if(o) return o->anim;
    Scene_Light* l = std::get_if<Scene_Light>(&data);
    if(l) return l->anim;
    Scene_Particles* p = std::get_if<Scene_Particles>(&data);
    if(p) return p->anim;

    assert(false);
    return l->anim;
}

const Anim_Pose& Scene_Item::animation() const {

    const Scene_Object* o = std::get_if<Scene_Object>(&data);
    if(o) return o->anim;
    const Scene_Light* l = std::get_if<Scene_Light>(&data);
    if(l) return l->anim;
    const Scene_Particles* p = std::get_if<Scene_Particles>(&data);
    if(p) return p->anim;

    assert(false);
    return l->anim;
}

Pose& Scene_Item::pose() {

    Scene_Object* o = std::get_if<Scene_Object>(&data);
    if(o) return o->pose;
    Scene_Light* l = std::get_if<Scene_Light>(&data);
    if(l) return l->pose;
    Scene_Particles* p = std::get_if<Scene_Particles>(&data);
    if(p) return p->pose;

    assert(false);
    return l->pose;
}

const Pose& Scene_Item::pose() const {

    const Scene_Object* o = std::get_if<Scene_Object>(&data);
    if(o) return o->pose;
    const Scene_Light* l = std::get_if<Scene_Light>(&data);
    if(l) return l->pose;
    const Scene_Particles* p = std::get_if<Scene_Particles>(&data);
    if(p) return p->pose;

    assert(false);
    return l->pose;
}

std::pair<char*, int> Scene_Item::name() {

    Scene_Object* o = std::get_if<Scene_Object>(&data);
    if(o) return {o->opt.name, Scene_Object::max_name_len};
    Scene_Light* l = std::get_if<Scene_Light>(&data);
    if(l) return {l->opt.name, Scene_Light::max_name_len};
    Scene_Particles* p = std::get_if<Scene_Particles>(&data);
    if(p) return {p->opt.name, Scene_Particles::max_name_len};

    assert(false);
    return {l->opt.name, Scene_Object::max_name_len};
}

std::string Scene_Item::name() const {

    const Scene_Object* o = std::get_if<Scene_Object>(&data);
    if(o) return std::string(o->opt.name);
    const Scene_Light* l = std::get_if<Scene_Light>(&data);
    if(l) return std::string(l->opt.name);
    const Scene_Particles* p = std::get_if<Scene_Particles>(&data);
    if(p) return std::string(p->opt.name);

    assert(false);
    return std::string(l->opt.name);
}

Scene::Scene(Scene_ID start) : next_id(start), first_id(start) {
}

Scene_ID Scene::used_ids() {
    return next_id;
}

Scene_ID Scene::reserve_id() {
    return next_id++;
}

Scene_ID Scene::add(Pose pose, Halfedge_Mesh&& mesh, std::string n, Scene_ID id) {
    if(!id) id = next_id++;
    assert(objs.find(id) == objs.end());
    objs.emplace(std::make_pair(id, Scene_Object(id, pose, std::move(mesh), n)));
    return id;
}

Scene_ID Scene::add(Pose pose, GL::Mesh&& mesh, std::string n, Scene_ID id) {
    if(!id) id = next_id++;
    assert(objs.find(id) == objs.end());
    objs.emplace(std::make_pair(id, Scene_Object(id, pose, std::move(mesh), n)));
    return id;
}

std::string Scene::set_env_map(std::string file) {
    Scene_ID id = 0;
    for_items([&id](const Scene_Item& item) {
        if(item.is<Scene_Light>() && item.get<Scene_Light>().is_env()) id = item.id();
    });
    Scene_Light l(Light_Type::sphere, reserve_id(), {}, "env_map");
    std::string err = l.emissive_load(file);
    if(err.empty()) {
        if(id) erase(id);
        add(std::move(l));
    }
    return err;
}

bool Scene::has_env_light() const {
    bool ret = false;
    for_items([&ret](const Scene_Item& item) {
        ret = ret || (item.is<Scene_Light>() && item.get<Scene_Light>().is_env());
    });
    return ret;
}

bool Scene::has_obj() const {
    bool ret = false;
    for_items([&ret](const Scene_Item& item) { ret = ret || item.is<Scene_Object>(); });
    return ret;
}

bool Scene::has_particles() const {
    bool ret = false;
    for_items([&ret](const Scene_Item& item) { ret = ret || item.is<Scene_Particles>(); });
    return ret;
}

Scene_ID Scene::add(Scene_Light&& obj) {
    assert(objs.find(obj.id()) == objs.end());
    objs.emplace(std::make_pair(obj.id(), std::move(obj)));
    return obj.id();
}

Scene_ID Scene::add(Scene_Object&& obj) {
    assert(objs.find(obj.id()) == objs.end());
    objs.emplace(std::make_pair(obj.id(), std::move(obj)));
    return obj.id();
}

Scene_ID Scene::add(Scene_Particles&& obj) {
    assert(objs.find(obj.id()) == objs.end());
    objs.emplace(std::make_pair(obj.id(), std::move(obj)));
    return obj.id();
}

void Scene::restore(Scene_ID id) {
    if(objs.find(id) != objs.end()) return;
    assert(erased.find(id) != erased.end());
    objs.insert({id, std::move(erased[id])});
    erased.erase(id);
}

void Scene::erase(Scene_ID id) {
    assert(erased.find(id) == erased.end());
    assert(objs.find(id) != objs.end());

    erased.insert({id, std::move(objs[id])});
    objs.erase(id);
}

void Scene::for_items(std::function<void(const Scene_Item&)> func) const {
    for(const auto& obj : objs) {
        func(obj.second);
    }
}

void Scene::for_items(std::function<void(Scene_Item&)> func) {
    for(auto& obj : objs) {
        func(obj.second);
    }
}

size_t Scene::size() {
    return objs.size();
}

bool Scene::empty() {
    return objs.size() == 0;
}

Scene_Maybe Scene::get(Scene_ID id) {
    auto entry = objs.find(id);
    if(entry == objs.end()) return std::nullopt;
    return entry->second;
}

Scene_Light& Scene::get_light(Scene_ID id) {
    auto entry = objs.find(id);
    assert(entry != objs.end());
    assert(entry->second.is<Scene_Light>());
    return entry->second.get<Scene_Light>();
}

Scene_Object& Scene::get_obj(Scene_ID id) {
    auto entry = objs.find(id);
    assert(entry != objs.end());
    assert(entry->second.is<Scene_Object>());
    return entry->second.get<Scene_Object>();
}

Scene_Particles& Scene::get_particles(Scene_ID id) {
    auto entry = objs.find(id);
    assert(entry != objs.end());
    assert(entry->second.is<Scene_Particles>());
    return entry->second.get<Scene_Particles>();
}

void Scene::clear(Undo& undo) {
    next_id = first_id;
    objs.clear();
    erased.clear();
    undo.reset();
}

//////////////////////////////////////////////////////////////
// Scene importer/exporter
//////////////////////////////////////////////////////////////

static const std::string FAKE_NAME = "FAKE-S3D-FAKE-MESH";

static const std::string RENDER_CAM_NODE = "S3D-RENDER_CAM_NODE";
static const std::string ANIM_CAM_NODE = "S3D-ANIM_CAM_NODE";
static const std::string ANIM_CAM_NAME = "S3D-ANIM_CAM";
static const std::string ANIM_NAME = "S3D-ANIMATION";

static const std::string FLIPPED_TAG = "FLIPPED";
static const std::string SMOOTHED_TAG = "SMOOTHED";
static const std::string SPHERESHAPE_TAG = "SPHERESHAPE";
static const std::string EMITTER_TAG = "EMITTER";
static const std::string EMITTER_ANIM = "EMITTER_ANIM_NODE";

static const std::string HEMISPHERE_TAG = "HEMISPHERE";
static const std::string SPHERE_TAG = "SPHERE";
static const std::string AREA_TAG = "AREA";
static const std::string LIGHT_ANIM = "LIGHT_ANIM_NODE";

static const std::string JOINT_TAG = "JOINT";
static const std::string IK_TAG = "JOINT-IK";
static const std::string ARMATURE_TAG = "ARMATURE";

static const std::string MAT_ANIM0 = "MAT_ANIM_NODE0";
static const std::string MAT_ANIM1 = "MAT_ANIM_NODE1";

static aiVector3D vecVec(Vec3 v) {
    return aiVector3D(v.x, v.y, v.z);
}

static Quat aiQuat(aiQuaternion aiv) {
    return Quat(aiv.x, aiv.y, aiv.z, aiv.w);
}

static Vec3 aiVec(aiVector3D aiv) {
    return Vec3(aiv.x, aiv.y, aiv.z);
}

static Spectrum aiSpec(aiColor3D aiv) {
    return Spectrum(aiv.r, aiv.g, aiv.b);
}

static aiMatrix4x4 matMat(const Mat4& T) {
    return {T[0][0], T[1][0], T[2][0], T[3][0], T[0][1], T[1][1], T[2][1], T[3][1],
            T[0][2], T[1][2], T[2][2], T[3][2], T[0][3], T[1][3], T[2][3], T[3][3]};
}

static Mat4 aiMat(aiMatrix4x4 T) {
    return Mat4{Vec4{T[0][0], T[1][0], T[2][0], T[3][0]}, Vec4{T[0][1], T[1][1], T[2][1], T[3][1]},
                Vec4{T[0][2], T[1][2], T[2][2], T[3][2]}, Vec4{T[0][3], T[1][3], T[2][3], T[3][3]}};
}

static aiMatrix4x4 node_transform(const aiNode* node) {
    aiMatrix4x4 T;
    while(node) {
        T = T * node->mTransformation;
        node = node->mParent;
    }
    return T;
}

static GL::Mesh mesh_from(const aiMesh* mesh) {

    std::vector<GL::Mesh::Vert> mesh_verts;
    std::vector<GL::Mesh::Index> mesh_inds;

    for(unsigned int j = 0; j < mesh->mNumVertices; j++) {
        const aiVector3D& vpos = mesh->mVertices[j];
        aiVector3D vnorm;
        if(mesh->HasNormals()) {
            vnorm = mesh->mNormals[j];
        }
        mesh_verts.push_back({aiVec(vpos), aiVec(vnorm), 0});
    }

    for(unsigned int j = 0; j < mesh->mNumFaces; j++) {
        const aiFace& face = mesh->mFaces[j];
        if(face.mNumIndices < 3) continue;
        unsigned int start = face.mIndices[0];
        for(size_t k = 1; k <= face.mNumIndices - 2; k++) {
            mesh_inds.push_back(start);
            mesh_inds.push_back(face.mIndices[k]);
            mesh_inds.push_back(face.mIndices[k + 1]);
        }
    }

    return GL::Mesh(std::move(mesh_verts), std::move(mesh_inds));
}

using vp_pair = std::pair<std::vector<Vec3>, std::vector<std::vector<Halfedge_Mesh::Index>>>;
static vp_pair load_mesh(const aiMesh* mesh) {

    std::vector<Vec3> verts;

    for(unsigned int j = 0; j < mesh->mNumVertices; j++) {
        const aiVector3D& pos = mesh->mVertices[j];
        verts.push_back(Vec3(pos.x, pos.y, pos.z));
    }

    std::vector<std::vector<Halfedge_Mesh::Index>> polys;
    for(unsigned int j = 0; j < mesh->mNumFaces; j++) {
        const aiFace& face = mesh->mFaces[j];
        if(face.mNumIndices < 3) continue;
        std::vector<Halfedge_Mesh::Index> poly;
        for(unsigned int k = 0; k < face.mNumIndices; k++) {
            poly.push_back(face.mIndices[k]);
        }
        polys.push_back(poly);
    }
    return {verts, polys};
}

static Scene_Particles::Options load_particles(aiLight* ai_light, aiNode* anim_node) {

    Scene_Particles::Options opt;
    aiColor3D color = ai_light->mColorAmbient;

    opt.color = Spectrum(color.r, color.g, color.b);
    opt.scale = ai_light->mAttenuationConstant;
    opt.velocity = ai_light->mAttenuationLinear;
    opt.enabled = ai_light->mAttenuationQuadratic > 0.0f;
    opt.angle = std::abs(ai_light->mAttenuationQuadratic);
    opt.pps = ai_light->mColorDiffuse.r;

    if(anim_node) {
        aiVector3D ascale, arot, apos;
        anim_node->mTransformation.Decompose(ascale, arot, apos);
        Vec3 info = aiVec(apos);
        opt.lifetime = info.x;
    }
    return opt;
}

static Scene_Light::Options load_light(aiLight* ai_light, bool hemi, bool sphere, bool area) {

    Scene_Light::Options opt;
    aiColor3D color;

    switch(ai_light->mType) {
    case aiLightSource_DIRECTIONAL: {
        opt.type = Light_Type::directional;
        color = ai_light->mColorDiffuse;
    } break;
    case aiLightSource_POINT: {
        opt.type = Light_Type::point;
        color = ai_light->mColorDiffuse;
    } break;
    case aiLightSource_SPOT: {
        opt.type = Light_Type::spot;
        opt.angle_bounds.x = Degrees(ai_light->mAngleInnerCone);
        opt.angle_bounds.y = Degrees(ai_light->mAngleOuterCone);
        color = ai_light->mColorDiffuse;
    } break;
    case aiLightSource_AMBIENT: {
        if(hemi)
            opt.type = Light_Type::hemisphere;
        else if(sphere) {
            opt.type = Light_Type::sphere;
            if(ai_light->mEnvMap.length) {
                opt.has_emissive_map = true;
            }
        } else if(area) {
            opt.type = Light_Type::rectangle;
            opt.size.x = ai_light->mAttenuationConstant;
            opt.size.y = ai_light->mAttenuationLinear;
        }
        color = ai_light->mColorAmbient;
    } break;
    case aiLightSource_AREA: {
        opt.type = Light_Type::rectangle;
        opt.size.x = ai_light->mSize.x;
        opt.size.y = ai_light->mSize.y;
        color = ai_light->mColorDiffuse;
    } break;
    default: break;
    }

    float power = std::max(color.r, std::max(color.g, color.b));
    opt.spectrum = Spectrum(color.r, color.g, color.b) * (1.0f / power);
    opt.intensity = power;
    return opt;
}

static Material::Options load_material(aiMaterial* ai_mat, float& was_sphere) {

    Material::Options mat;

    aiColor3D albedo;
    ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
    mat.albedo = aiSpec(albedo);

    aiColor3D emissive;
    ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
    mat.emissive = aiSpec(emissive);

    aiColor3D reflectance;
    ai_mat->Get(AI_MATKEY_COLOR_REFLECTIVE, reflectance);
    mat.reflectance = aiSpec(reflectance);

    aiColor3D transmittance;
    ai_mat->Get(AI_MATKEY_COLOR_TRANSPARENT, transmittance);
    mat.transmittance = aiSpec(transmittance);

    ai_mat->Get(AI_MATKEY_REFRACTI, mat.ior);
    ai_mat->Get(AI_MATKEY_SHININESS, mat.intensity);

    aiString ai_type;
    ai_mat->Get(AI_MATKEY_NAME, ai_type);
    std::string type(ai_type.C_Str());

    if(type.find("lambertian") != std::string::npos) {
        mat.type = Material_Type::lambertian;
    } else if(type.find("mirror") != std::string::npos) {
        mat.type = Material_Type::mirror;
    } else if(type.find("refract") != std::string::npos) {
        mat.type = Material_Type::refract;
    } else if(type.find("glass") != std::string::npos) {
        mat.type = Material_Type::glass;
    } else if(type.find("diffuse_light") != std::string::npos) {
        mat.type = Material_Type::diffuse_light;
    } else {
        mat = Material::Options();
    }
    mat.emissive *= 1.0f / mat.intensity;

    if(type.find(SPHERESHAPE_TAG) != std::string::npos) {
        aiColor3D specular;
        ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        was_sphere = specular.r;
    }

    return mat;
}

static void load_node(Scene& scobj, std::vector<std::string>& errors,
                      std::unordered_map<aiNode*, Scene_ID>& node_to_obj,
                      std::unordered_map<aiNode*, Joint*>& node_to_bone,
                      std::unordered_map<aiNode*, Skeleton::IK_Handle*>& node_to_ik,
                      const aiScene* scene, aiNode* node, aiMatrix4x4 transform) {

    transform = transform * node->mTransformation;

    for(unsigned int i = 0; i < node->mNumMeshes; i++) {

        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        std::string name;
        bool do_flip = false, do_smooth = false;

        if(mesh->mName.length) {
            name = std::string(mesh->mName.C_Str());

            if(name.find(FAKE_NAME) != std::string::npos) continue;

            size_t special = name.find("-S3D-");
            if(special != std::string::npos) {
                if(name.find(FLIPPED_TAG) != std::string::npos) do_flip = true;
                if(name.find(SMOOTHED_TAG) != std::string::npos) do_smooth = true;
                if(name.find(EMITTER_TAG) != std::string::npos) continue;
                name = name.substr(0, special);
                std::replace(name.begin(), name.end(), '_', ' ');
            }
        }

        auto [verts, polys] = load_mesh(mesh);

        aiVector3D ascale, arot, apos;
        transform.Decompose(ascale, arot, apos);
        Vec3 pos = aiVec(apos);
        Vec3 rot = aiVec(arot);
        Vec3 scale = aiVec(ascale);
        Pose p = {pos, Degrees(rot).range(0.0f, 360.0f), scale};

        float was_sphere = -1.0f;
        Material::Options mat_opt =
            load_material(scene->mMaterials[mesh->mMaterialIndex], was_sphere);

        Scene_Object new_obj;

        if(was_sphere > 0.0f) {

            Scene_Object obj(scobj.reserve_id(), p, GL::Mesh(), name);
            obj.opt.shape_type = PT::Shape_Type::sphere;
            obj.opt.shape = PT::Shape(PT::Sphere(was_sphere));
            new_obj = std::move(obj);

        } else {

            Halfedge_Mesh hemesh;
            std::string err = hemesh.from_poly(polys, verts);
            if(!err.empty()) {

                GL::Mesh gmesh = mesh_from(mesh);
                errors.push_back(err);
                Scene_Object obj(scobj.reserve_id(), p, std::move(gmesh), name);
                new_obj = std::move(obj);

            } else {

                if(do_flip) hemesh.flip();
                Scene_Object obj(scobj.reserve_id(), p, std::move(hemesh), name);
                obj.opt.smooth_normals = do_smooth;
                new_obj = std::move(obj);
            }
        }

        new_obj.material.opt = mat_opt;

        if(mesh->mNumBones) {

            Skeleton& skeleton = new_obj.armature;
            aiNode* arm_node = mesh->mBones[0]->mArmature;
            if(arm_node) {
                {
                    aiVector3D t, r, s;
                    arm_node->mTransformation.Decompose(s, r, t);
                    skeleton.base() = aiVec(t);
                }

                std::unordered_map<aiNode*, aiBone*> node_to_aibone;
                for(unsigned int j = 0; j < mesh->mNumBones; j++) {
                    node_to_aibone[mesh->mBones[j]->mNode] = mesh->mBones[j];
                }

                std::function<void(Joint*, aiNode*)> build_tree;
                build_tree = [&](Joint* p, aiNode* node) {
                    aiBone* bone = node_to_aibone[node];
                    aiVector3D t, r, s;
                    bone->mOffsetMatrix.Decompose(s, r, t);

                    std::string name(bone->mName.C_Str());
                    if(name.find(IK_TAG) != std::string::npos) {
                        Skeleton::IK_Handle* h = skeleton.add_handle(aiVec(t), p);
                        h->enabled = bone->mWeights[0].mWeight > 1.0f;
                        node_to_ik[node] = h;
                    } else {
                        Joint* c = skeleton.add_child(p, aiVec(t));
                        node_to_bone[node] = c;
                        c->pose = aiVec(r);
                        c->radius = bone->mWeights[0].mWeight;
                        for(unsigned int j = 0; j < node->mNumChildren; j++)
                            build_tree(c, node->mChildren[j]);
                    }
                };
                for(unsigned int j = 0; j < arm_node->mNumChildren; j++) {
                    aiNode* root_node = arm_node->mChildren[j];
                    aiBone* root_bone = node_to_aibone[root_node];
                    aiVector3D t, r, s;
                    root_bone->mOffsetMatrix.Decompose(s, r, t);
                    Joint* root = skeleton.add_root(aiVec(t));
                    node_to_bone[root_node] = root;
                    root->pose = aiVec(r);
                    root->radius = root_bone->mWeights[0].mWeight;
                    for(unsigned int k = 0; k < root_node->mNumChildren; k++)
                        build_tree(root, root_node->mChildren[k]);
                }
            }
        }

        std::string m0 = std::string(node->mName.C_Str()) + "-MAT_ANIM_NODE0";
        aiNode* m0_node = scene->mRootNode->FindNode(aiString(m0));
        if(m0_node) {
            node_to_obj[m0_node] = new_obj.id();
        }

        std::string m1 = std::string(node->mName.C_Str()) + "-MAT_ANIM_NODE1";
        aiNode* m1_node = scene->mRootNode->FindNode(aiString(m1));
        if(m1_node) {
            node_to_obj[m1_node] = new_obj.id();
        }

        node_to_obj[node] = new_obj.id();
        scobj.add(std::move(new_obj));
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        load_node(scobj, errors, node_to_obj, node_to_bone, node_to_ik, scene, node->mChildren[i],
                  transform);
    }
}

static unsigned int load_flags(Scene::Load_Opts opt) {

    unsigned int flags = aiProcess_OptimizeMeshes | aiProcess_FindInvalidData |
                         aiProcess_FindInstances | aiProcess_FindDegenerates;

    if(opt.drop_normals) {
        flags |= aiProcess_DropNormals;
    }
    if(opt.join_verts) {
        flags |= aiProcess_JoinIdenticalVertices;
    }
    if(opt.triangulate) {
        flags |= aiProcess_Triangulate;
    }
    if(opt.gen_smooth_normals) {
        flags |= aiProcess_GenSmoothNormals;
    } else if(opt.gen_normals) {
        flags |= aiProcess_GenNormals;
    }
    if(opt.fix_infacing_normals) {
        flags |= aiProcess_FixInfacingNormals;
    }
    if(opt.debone) {
        flags |= aiProcess_Debone;
    } else {
        flags |= aiProcess_PopulateArmatureData;
    }

    return flags;
}

std::string Scene::load(Scene::Load_Opts loader, Undo& undo, Gui::Manager& gui, std::string file) {

    if(loader.new_scene) {
        clear(undo);
        gui.get_animate().clear();
        gui.get_rig().clear();
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file.c_str(), load_flags(loader));

    if(!scene) {
        return "Parsing scene " + file + ": " + std::string(importer.GetErrorString());
    }

    std::vector<std::string> errors;
    std::unordered_map<aiNode*, Scene_ID> node_to_obj;
    std::unordered_map<aiNode*, Joint*> node_to_bone;
    std::unordered_map<aiNode*, Skeleton::IK_Handle*> node_to_ik;
    scene->mRootNode->mTransformation = aiMatrix4x4();

    // Load objects
    load_node(*this, errors, node_to_obj, node_to_bone, node_to_ik, scene, scene->mRootNode,
              aiMatrix4x4());

    // Load cameras
    if(loader.new_scene && scene->mNumCameras > 0) {

        auto load_cam = [&](unsigned int i) {
            const aiCamera& aiCam = *scene->mCameras[i];
            Mat4 cam_transform = aiMat(node_transform(scene->mRootNode->FindNode(aiCam.mName)));
            Vec3 pos = cam_transform * aiVec(aiCam.mPosition);
            Vec3 center = cam_transform * aiVec(aiCam.mLookAt);

            std::string name(aiCam.mName.C_Str());
            if(name.find(ANIM_CAM_NAME) != std::string::npos) {
                gui.get_animate().load_cam(pos, center, aiCam.mAspect, aiCam.mHorizontalFOV,
                                           aiCam.mClipPlaneNear, aiCam.mClipPlaneFar);
            } else {
                gui.get_render().load_cam(pos, center, aiCam.mAspect, aiCam.mHorizontalFOV,
                                          aiCam.mClipPlaneNear, aiCam.mClipPlaneFar);
            }
        };

        for(unsigned int i = 0; i < scene->mNumCameras; i++) {
            load_cam(i);
        }
    }

    // Load lights and particle emitters
    for(unsigned int i = 0; i < scene->mNumLights; i++) {

        aiLight* ai_light = scene->mLights[i];
        aiNode* node = scene->mRootNode->FindNode(ai_light->mName);

        Pose p;
        {
            Mat4 trans = aiMat(node_transform(node));
            p.pos = trans * aiVec(ai_light->mPosition);
            p.euler = trans.to_euler();
        }

        bool was_exported_from_s3d = false;
        float export_power = 1.0f;
        bool is_sphere = false, is_hemisphere = false, is_area = false, is_emitter = false;

        std::string name = std::string(node->mName.C_Str());

        size_t special = name.find("-S3D-");
        if(special != std::string::npos) {

            was_exported_from_s3d = true;
            export_power = ai_light->mAttenuationQuadratic;

            std::string left = name.substr(special + 4);
            name = name.substr(0, special);
            std::replace(name.begin(), name.end(), '_', ' ');

            if(left.find(HEMISPHERE_TAG) != std::string::npos)
                is_hemisphere = true;
            else if(left.find(SPHERE_TAG) != std::string::npos)
                is_sphere = true;
            else if(left.find(AREA_TAG) != std::string::npos)
                is_area = true;
            else if(left.find(EMITTER_TAG) != std::string::npos)
                is_emitter = true;
        }

        if(is_emitter) {

            Scene_Particles particles(reserve_id(), p, name);
            node_to_obj[node] = particles.id();

            std::string anim_name = std::string(node->mName.C_Str()) + "-" + EMITTER_ANIM;
            aiNode* anim_node = scene->mRootNode->FindNode(aiString(anim_name));
            if(anim_node) {
                node_to_obj[anim_node] = particles.id();

                if(anim_node->mNumMeshes > 0) {
                    aiMesh* mesh = scene->mMeshes[anim_node->mMeshes[0]];
                    particles.take_mesh(mesh_from(mesh));
                }
            }

            Scene_Particles::Options opt = load_particles(ai_light, anim_node);
            snprintf(opt.name, Scene_Particles::max_name_len, "%s", name.c_str());
            particles.opt = opt;

            add(std::move(particles));

        } else {

            aiColor3D color;
            Scene_Light light(Light_Type::point, reserve_id(), p, name);

            Scene_Light::Options opt = load_light(ai_light, is_hemisphere, is_sphere, is_area);
            if(was_exported_from_s3d) {
                float factor = opt.intensity / export_power;
                opt.spectrum *= factor;
                opt.intensity = export_power;
            }

            snprintf(opt.name, Scene_Object::max_name_len, "%s", name.c_str());
            light.opt = opt;

            if(light.opt.has_emissive_map) {
                light.emissive_load(std::string(ai_light->mEnvMap.C_Str()));
            }

            if(!light.is_env() || !has_env_light()) {

                node_to_obj[node] = light.id();

                std::string anim_name = std::string(node->mName.C_Str()) + "-" + LIGHT_ANIM;
                aiNode* anim_node = scene->mRootNode->FindNode(aiString(anim_name));
                if(anim_node) {
                    node_to_obj[anim_node] = light.id();
                }

                add(std::move(light));
            }
        }
    }

    bool loaded = false;
    auto load_anim = [&loaded](aiNodeAnim* node, std::string name, auto set_frame) {
        if(loaded) return;
        std::string node_name(node->mNodeName.C_Str());
        if(node_name.find(name) == std::string::npos) return;

        loaded = true;
        unsigned int keys = std::min(node->mNumPositionKeys,
                                     std::min(node->mNumRotationKeys, node->mNumScalingKeys));

        for(unsigned int k = 0; k < keys; k++) {
            float t = (float)node->mPositionKeys[k].mTime;
            Vec3 pos = aiVec(node->mPositionKeys[k].mValue);
            Quat rot = aiQuat(node->mRotationKeys[k].mValue);
            Vec3 scl = aiVec(node->mScalingKeys[k].mValue);
            set_frame(t, pos, rot, scl);
        }
    };

    auto get_id = [&](aiNodeAnim* node) -> Scene_ID {
        aiNode* obj_node = scene->mRootNode->FindNode(node->mNodeName);
        auto entry = node_to_obj.find(obj_node);
        if(entry == node_to_obj.end()) return 0;
        return entry->second;
    };

    auto get_bone = [&](aiNodeAnim* node) -> Joint* {
        aiNode* obj_node = scene->mRootNode->FindNode(node->mNodeName);
        auto entry = node_to_bone.find(obj_node);
        if(entry == node_to_bone.end()) return nullptr;
        return entry->second;
    };

    auto get_ik = [&](aiNodeAnim* node) -> Skeleton::IK_Handle* {
        aiNode* obj_node = scene->mRootNode->FindNode(node->mNodeName);
        auto entry = node_to_ik.find(obj_node);
        if(entry == node_to_ik.end()) return nullptr;
        return entry->second;
    };

    Gui::Anim_Camera& cam = gui.get_animate().camera();
    float ar = gui.get_render().get_cam().get_ar();

    // Load animation data
    for(unsigned int i = 0; i < scene->mNumAnimations; i++) {

        aiAnimation* anim = scene->mAnimations[i];
        for(unsigned int j = 0; j < anim->mNumChannels; j++) {

            aiNodeAnim* node = anim->mChannels[j];
            loaded = false;

            // Load animated camera
            load_anim(node, ANIM_CAM_NODE, [&cam, ar](float t, Vec3 p, Quat q, Vec3 s) {
                cam.splines.set(t, p, q, s.x, ar, s.y - 1.0f, s.z);
            });

            // Load animated bones
            Joint* jt = get_bone(node);
            if(jt) {
                load_anim(node, JOINT_TAG,
                          [jt](float t, Vec3 p, Quat q, Vec3 s) { jt->anim.set(t, q); });
            }

            // Load animated IK handles
            Skeleton::IK_Handle* h = get_ik(node);
            if(h) {
                load_anim(node, IK_TAG,
                          [h](float t, Vec3 p, Quat q, Vec3 s) { h->anim.set(t, p, s.x > 1.0f); });
            }

            Scene_ID id = get_id(node);
            if(!id) continue;

            // Load animated light
            load_anim(node, LIGHT_ANIM, [&](float t, Vec3 p, Quat q, Vec3 s) {
                Vec3 a = q.to_euler();
                Scene_Light::Anim_Light& light = get_light(id).lanim;
                light.splines.set(t, Spectrum{p.x, p.y, p.z}, s.x - 1.0f, Vec2{a.x, a.z},
                                  Vec2{s.y - 1.0f, s.z - 1.0f});
            });

            // Load animated particle emitter
            load_anim(node, EMITTER_ANIM, [&](float t, Vec3 p, Quat q, Vec3 s) {
                Scene_Particles::Anim_Particles& particles = get_particles(id).panim;
                Vec3 a = q.to_euler();
                particles.splines.set(t, Spectrum{p.x, p.y, p.z}, s.z - 1.0f, a.z, a.x, s.y - 1.0f,
                                      std::abs(s.x) - 1.0f, s.x > 0.0f);
            });

            // Load animated material part 1
            load_anim(node, MAT_ANIM0, [&](float t, Vec3 p, Quat q, Vec3 s) {
                Material::Anim_Material& mat = get_obj(id).material.anim;
                Vec3 a = q.to_euler();
                Vec3 r = s - Vec3{1.0f};
                Material::Options opts;
                mat.at(t, opts);
                opts.albedo = Spectrum(p.x, p.y, p.z);
                opts.reflectance = Spectrum(r.x, r.y, r.z);
                opts.emissive.r = a.x;
                mat.set(t, opts);
            });

            // Load animated material part 2
            load_anim(node, MAT_ANIM1, [&](float t, Vec3 p, Quat q, Vec3 s) {
                Material::Anim_Material& mat = get_obj(id).material.anim;
                Vec3 a = q.to_euler();
                Vec3 v = s - Vec3{1.0f};
                Material::Options opts;
                mat.at(t, opts);
                opts.transmittance = Spectrum(p.x, p.y, p.z);
                opts.emissive.g = a.x;
                opts.emissive.b = v.z;
                opts.intensity = v.x;
                opts.ior = v.y;
                mat.set(t, opts);
            });

            // Load animated pose (all objects)
            load_anim(node, "", [&](float t, Vec3 p, Quat q, Vec3 s) {
                Anim_Pose& pose = (*get(id)).get().animation();
                pose.splines.set(t, p, q, s);
            });
        }

        if(anim->mDuration > 0.0f) {
            gui.get_animate().set((int)std::ceil(anim->mDuration),
                                  (int)std::round(anim->mTicksPerSecond));
        }
    }
    gui.get_animate().refresh(*this);

    std::stringstream stream;
    if(errors.size()) {
        stream << "Meshes with errors may not be edit-able in the model mode." << std::endl
               << std::endl;
    }
    for(size_t i = 0; i < errors.size(); i++) {
        stream << "Loading mesh " << i << ": " << errors[i] << std::endl;
    }
    return stream.str();
}

static void write_particles(aiLight* ai_light, const Scene_Particles::Options& opt,
                            std::string name) {

    Spectrum r = opt.color;
    ai_light->mType = aiLightSource_AMBIENT;
    ai_light->mName = aiString(name);
    ai_light->mPosition = aiVector3D(0.0f, 0.0f, 0.0f);
    ai_light->mDirection = aiVector3D(0.0f, 1.0f, 0.0f);
    ai_light->mUp = aiVector3D(0.0f, 1.0f, 0.0f);
    ai_light->mColorAmbient = aiColor3D(r.r, r.g, r.b);
    ai_light->mColorDiffuse = aiColor3D(opt.pps, 0.0f, 0.0f);
    ai_light->mAttenuationConstant = opt.scale;
    ai_light->mAttenuationLinear = opt.velocity;
    ai_light->mAttenuationQuadratic = opt.enabled ? opt.angle : -opt.angle;
}

static std::string write_light(aiLight* ai_light, const Scene_Light::Options& opt, std::string name,
                               Scene_ID id, std::string map) {

    Spectrum r = opt.spectrum * opt.intensity;

    switch(opt.type) {
    case Light_Type::directional: {
        ai_light->mType = aiLightSource_DIRECTIONAL;
        ai_light->mColorDiffuse = aiColor3D(r.r, r.g, r.b);
        name += "-S3D-" + std::to_string(id);
    } break;
    case Light_Type::sphere: {
        ai_light->mType = aiLightSource_AMBIENT;
        ai_light->mColorAmbient = aiColor3D(r.r, r.g, r.b);
        name += "-S3D-" + SPHERE_TAG + "-" + std::to_string(id);
        if(opt.has_emissive_map) {
            ai_light->mEnvMap = aiString(map);
        }
    } break;
    case Light_Type::hemisphere: {
        ai_light->mType = aiLightSource_AMBIENT;
        ai_light->mColorAmbient = aiColor3D(r.r, r.g, r.b);
        name += "-S3D-" + HEMISPHERE_TAG + "-" + std::to_string(id);
    } break;
    case Light_Type::point: {
        ai_light->mType = aiLightSource_POINT;
        ai_light->mColorDiffuse = aiColor3D(r.r, r.g, r.b);
        name += "-S3D-" + std::to_string(id);
    } break;
    case Light_Type::spot: {
        ai_light->mType = aiLightSource_SPOT;
        ai_light->mColorDiffuse = aiColor3D(r.r, r.g, r.b);
        name += "-S3D-" + std::to_string(id);
    } break;
    case Light_Type::rectangle: {
        // the collada exporter literally just ignores area lights ????????
        ai_light->mType = aiLightSource_AMBIENT;
        ai_light->mColorAmbient = aiColor3D(r.r, r.g, r.b);
        name += "-S3D-" + AREA_TAG + "-" + std::to_string(id);
        ai_light->mAttenuationConstant = opt.size.x;
        ai_light->mAttenuationLinear = opt.size.y;
    } break;
    default: break;
    }

    ai_light->mName = aiString(name);
    ai_light->mPosition = aiVector3D(0.0f, 0.0f, 0.0f);
    ai_light->mDirection = aiVector3D(0.0f, 1.0f, 0.0f);
    ai_light->mUp = aiVector3D(0.0f, 1.0f, 0.0f);
    ai_light->mAngleInnerCone = Radians(opt.angle_bounds.x);
    ai_light->mAngleOuterCone = Radians(opt.angle_bounds.y);
    ai_light->mAttenuationQuadratic = opt.intensity;
    return name;
}

static void write_material(aiMaterial* ai_mat, const Material::Options& opt, float is_sphere) {

    std::string mat_name;
    switch(opt.type) {
    case Material_Type::lambertian: {
        mat_name = "lambertian";
    } break;
    case Material_Type::mirror: {
        mat_name = "mirror";
    } break;
    case Material_Type::refract: {
        mat_name = "refract";
    } break;
    case Material_Type::glass: {
        mat_name = "glass";
    } break;
    case Material_Type::diffuse_light: {
        mat_name = "diffuse_light";
    } break;
    default: break;
    }

    // Horrible hack
    if(is_sphere >= 0.0f) {
        mat_name += "-" + SPHERESHAPE_TAG;
        ai_mat->AddProperty(new aiColor3D(is_sphere), 1, AI_MATKEY_COLOR_SPECULAR);
    }

    Spectrum emissive = opt.emissive * opt.intensity;

    ai_mat->AddProperty(new aiString(mat_name), AI_MATKEY_NAME);
    ai_mat->AddProperty(new aiColor3D(opt.albedo.r, opt.albedo.g, opt.albedo.b), 1,
                        AI_MATKEY_COLOR_DIFFUSE);
    ai_mat->AddProperty(new aiColor3D(opt.reflectance.r, opt.reflectance.g, opt.reflectance.b), 1,
                        AI_MATKEY_COLOR_REFLECTIVE);
    ai_mat->AddProperty(
        new aiColor3D(opt.transmittance.r, opt.transmittance.g, opt.transmittance.b), 1,
        AI_MATKEY_COLOR_TRANSPARENT);
    ai_mat->AddProperty(new aiColor3D(emissive.r, emissive.g, emissive.b), 1,
                        AI_MATKEY_COLOR_EMISSIVE);
    ai_mat->AddProperty(new float(opt.ior), 1, AI_MATKEY_REFRACTI);
    ai_mat->AddProperty(new float(opt.intensity), 1, AI_MATKEY_SHININESS);
}

static void write_hemesh(aiMesh* ai_mesh, const Halfedge_Mesh& mesh) {

    size_t n_verts = mesh.n_vertices();
    size_t n_faces = mesh.n_faces() - mesh.n_boundaries();

    ai_mesh->mVertices = new aiVector3D[n_verts];
    ai_mesh->mNumVertices = (unsigned int)n_verts;

    ai_mesh->mFaces = new aiFace[n_faces];
    ai_mesh->mNumFaces = (unsigned int)n_faces;

    std::unordered_map<size_t, size_t> id_to_idx;

    size_t vert_idx = 0;
    for(auto v = mesh.vertices_begin(); v != mesh.vertices_end(); v++) {
        id_to_idx[v->id()] = vert_idx;
        ai_mesh->mVertices[vert_idx] = vecVec(v->pos);
        vert_idx++;
    }

    size_t face_idx = 0;
    for(auto f = mesh.faces_begin(); f != mesh.faces_end(); f++) {

        if(f->is_boundary()) continue;

        aiFace& face = ai_mesh->mFaces[face_idx];
        face.mIndices = new unsigned int[f->degree()];
        face.mNumIndices = f->degree();

        size_t i_idx = 0;
        auto h = f->halfedge();
        do {
            face.mIndices[i_idx] = (unsigned int)id_to_idx[h->vertex()->id()];
            h = h->next();
            i_idx++;
        } while(h != f->halfedge());

        face_idx++;
    }
}

static void write_mesh(aiMesh* ai_mesh, const GL::Mesh& mesh) {
    const auto& verts = mesh.verts();
    const auto& elems = mesh.indices();

    ai_mesh->mVertices = new aiVector3D[verts.size()];
    ai_mesh->mNormals = new aiVector3D[verts.size()];
    ai_mesh->mNumVertices = (unsigned int)verts.size();

    int j = 0;
    for(GL::Mesh::Vert v : verts) {
        ai_mesh->mVertices[j] = vecVec(v.pos);
        ai_mesh->mNormals[j] = vecVec(v.norm);
        j++;
    }

    ai_mesh->mFaces = new aiFace[elems.size() / 3];
    ai_mesh->mNumFaces = (unsigned int)(elems.size() / 3);

    for(size_t i = 0; i < elems.size(); i += 3) {
        aiFace& face = ai_mesh->mFaces[i / 3];
        face.mIndices = new unsigned int[3];
        face.mNumIndices = 3;
        face.mIndices[0] = elems[i];
        face.mIndices[1] = elems[i + 1];
        face.mIndices[2] = elems[i + 2];
    }
}

static void write_cam(aiCamera* ai_cam, const Camera& cam, std::string name) {
    ai_cam->mAspect = cam.get_ar();
    ai_cam->mClipPlaneNear = cam.get_ap();
    ai_cam->mClipPlaneFar = cam.get_dist();
    ai_cam->mLookAt = aiVector3D(0.0f, 0.0f, -1.0f);
    ai_cam->mHorizontalFOV = Radians(cam.get_h_fov());
    ai_cam->mName = aiString(name);
}

static void add_fake_mesh(aiScene* scene) {
    aiMesh* ai_mesh = new aiMesh();
    aiNode* ai_node = new aiNode();
    scene->mMeshes[0] = ai_mesh;
    scene->mRootNode->mChildren[0] = ai_node;
    ai_node->mNumMeshes = 1;
    ai_node->mMeshes = new unsigned int((unsigned int)0);

    ai_mesh->mVertices = new aiVector3D[3];
    ai_mesh->mNumVertices = (unsigned int)3;
    ai_mesh->mVertices[0] = vecVec(Vec3{1.0f, 0.0f, 0.0f});
    ai_mesh->mVertices[1] = vecVec(Vec3{0.0f, 1.0f, 0.0f});
    ai_mesh->mVertices[2] = vecVec(Vec3{0.0f, 0.0f, 1.0f});

    ai_mesh->mFaces = new aiFace[1];
    ai_mesh->mNumFaces = 1u;
    ai_mesh->mNumBones = 0;
    ai_mesh->mBones = nullptr;

    ai_mesh->mFaces[0].mIndices = new unsigned int[3];
    ai_mesh->mFaces[0].mNumIndices = 3;
    ai_mesh->mFaces[0].mIndices[0] = 0;
    ai_mesh->mFaces[0].mIndices[1] = 1;
    ai_mesh->mFaces[0].mIndices[2] = 2;

    ai_mesh->mName = aiString(FAKE_NAME);
    ai_node->mName = aiString(FAKE_NAME);
    scene->mMaterials[0] = new aiMaterial();
}

Scene::Stats Scene::get_stats(const Gui::Animate& animation) {

    Stats s;
    for_items([&](Scene_Item& item) {
        if(item.is<Scene_Object>()) {

            Scene_Object& obj = item.get<Scene_Object>();

            s.objs++;
            s.meshes++;
            s.nodes++;

            if(obj.anim.splines.any()) {
                s.anims++;
            }
            if(obj.material.anim.splines.any()) {
                s.anims += 2;
                s.nodes += 2;
            }
            obj.armature.for_joints([&](Joint* j) {
                if(j->anim.any()) s.anims++;
            });
            obj.armature.for_handles([&](Skeleton::IK_Handle* h) {
                if(h->anim.any()) s.anims++;
            });

            if(obj.armature.has_bones()) {
                s.armatures++;
                s.nodes++;
            }

        } else if(item.is<Scene_Light>()) {

            Scene_Light& light = item.get<Scene_Light>();
            s.lights++;
            s.nodes++;

            if(light.anim.splines.any()) {
                s.anims++;
            }
            if(light.lanim.splines.any()) {
                s.anims++;
                s.nodes++;
            }

        } else if(item.is<Scene_Particles>()) {

            Scene_Particles& particles = item.get<Scene_Particles>();

            s.emitters++;
            s.meshes++;
            s.nodes++;

            if(particles.anim.splines.any()) {
                s.anims++;
                s.nodes++;
            }
            if(particles.panim.splines.any()) {
                s.anims++;
            }
        }
    });

    if(animation.camera().splines.any()) {
        s.anims++;
    }

    s.nodes += 2;
    return s;
}

std::string Scene::write(std::string file, const Camera& render_cam,
                         const Gui::Animate& animation) {

    size_t mesh_idx = 0, light_idx = 0, node_idx = 0, anim_idx = 0;
    Stats N = get_stats(animation);

    bool fake_mesh = N.meshes == 0;
    if(fake_mesh) {
        N.meshes++;
        N.nodes++;
    }

    aiScene scene;
    { // Scene Setup
        scene.mRootNode = new aiNode();

        scene.mRootNode->mNumMeshes = 0;
        scene.mRootNode->mNumChildren = N.nodes;
        scene.mRootNode->mChildren = new aiNode*[N.nodes]();

        scene.mMaterials = new aiMaterial*[N.meshes]();
        scene.mNumMaterials = N.meshes;

        scene.mMeshes = new aiMesh*[N.meshes]();
        scene.mNumMeshes = N.meshes;

        scene.mLights = new aiLight*[N.lights + N.emitters]();
        scene.mNumLights = N.lights + N.emitters;

        scene.mNumAnimations = 1;
        scene.mAnimations = new aiAnimation*[1];
        scene.mAnimations[0] = new aiAnimation();

        if(fake_mesh) {
            add_fake_mesh(&scene);
            node_idx++;
            mesh_idx++;
        }

        aiAnimation* ai_anim = scene.mAnimations[0];
        ai_anim->mName = aiString(ANIM_NAME);
        ai_anim->mDuration = (double)animation.n_frames();
        ai_anim->mTicksPerSecond = (double)animation.fps();
        ai_anim->mNumChannels = N.anims;
        ai_anim->mChannels = new aiNodeAnim*[N.anims];
    }

    { // Cameras
        scene.mCameras = new aiCamera*[2]();
        aiCamera* ar_cam = new aiCamera();
        aiCamera* aa_cam = new aiCamera();
        scene.mCameras[0] = ar_cam;
        scene.mCameras[1] = aa_cam;
        write_cam(ar_cam, render_cam, RENDER_CAM_NODE);
        write_cam(aa_cam, animation.current_camera(), ANIM_CAM_NODE);
        scene.mNumCameras = 2;

        size_t r_cam_idx = N.nodes - 1;
        size_t a_cam_idx = N.nodes - 2;

        Mat4 view = render_cam.get_view().inverse();
        scene.mRootNode->mChildren[r_cam_idx] = new aiNode();
        scene.mRootNode->mChildren[r_cam_idx]->mNumMeshes = 0;
        scene.mRootNode->mChildren[r_cam_idx]->mName = aiString(RENDER_CAM_NODE);
        scene.mRootNode->mChildren[r_cam_idx]->mTransformation = matMat(view);

        view = animation.current_camera().get_view().inverse();
        scene.mRootNode->mChildren[a_cam_idx] = new aiNode();
        scene.mRootNode->mChildren[a_cam_idx]->mNumMeshes = 0;
        scene.mRootNode->mChildren[a_cam_idx]->mName = aiString(ANIM_CAM_NODE);
        scene.mRootNode->mChildren[a_cam_idx]->mTransformation = matMat(view);
    }

    auto add_node = [&](std::string name) {
        aiNode* ai_node = new aiNode();
        scene.mRootNode->mChildren[node_idx++] = ai_node;
        ai_node->mName = aiString(name);
        ai_node->mNumMeshes = 0;
        ai_node->mMeshes = nullptr;
        ai_node->mTransformation = aiMatrix4x4();
    };

    std::unordered_map<Scene_ID, aiNode*> item_nodes;
    std::unordered_map<std::pair<Scene_ID, Scene_ID>, aiNode*> bone_nodes;

    for(auto& entry : objs) { // Scene Objects

        if(entry.second.is<Scene_Object>()) {

            Scene_Object& obj = entry.second.get<Scene_Object>();

            if(obj.is_shape()) {
                obj.try_make_editable(obj.opt.shape_type);
            }

            aiMesh* ai_mesh = new aiMesh();
            aiNode* ai_node = new aiNode();
            aiMaterial* ai_mat = new aiMaterial();

            size_t idx = mesh_idx++;
            scene.mMaterials[idx] = ai_mat;
            scene.mMeshes[idx] = ai_mesh;
            scene.mRootNode->mChildren[node_idx++] = ai_node;

            std::string name(obj.opt.name);
            {
                std::replace(name.begin(), name.end(), ' ', '_');
                name += "-S3D-" + std::to_string(obj.id());

                if(obj.get_mesh().flipped()) name += "-" + FLIPPED_TAG;
                if(obj.opt.smooth_normals) name += "-" + SMOOTHED_TAG;
            }

            ai_mesh->mName = aiString(name);
            ai_mesh->mMaterialIndex = (unsigned int)idx;

            Mat4 trans = obj.pose.transform();
            ai_node->mName = aiString(name);
            ai_node->mNumMeshes = 1;
            ai_node->mMeshes = new unsigned int((unsigned int)idx);
            ai_node->mTransformation = matMat(trans);
            item_nodes[obj.id()] = ai_node;

            if(obj.is_editable()) {
                write_hemesh(ai_mesh, obj.get_mesh());
            } else {
                write_mesh(ai_mesh, obj.mesh());
            }

            float r = -1.0f;
            if(obj.opt.shape_type == PT::Shape_Type::sphere) {
                r = obj.opt.shape.get<PT::Sphere>().radius;
            }
            write_material(ai_mat, obj.material.opt, r);

            if(obj.armature.has_bones()) {

                ai_mesh->mNumBones = obj.armature.n_bones() + obj.armature.n_handles();
                ai_mesh->mBones = new aiBone*[ai_mesh->mNumBones];

                size_t bone_idx = 0;
                std::string jprefix = "S3D-" + JOINT_TAG + "-" + std::to_string(obj.id()) + "-";
                std::string ikprefix = "S3D-" + IK_TAG + "-" + std::to_string(obj.id()) + "-";

                aiNode* arm_node = new aiNode();
                scene.mRootNode->mChildren[node_idx++] = arm_node;
                arm_node->mName = aiString(jprefix + ARMATURE_TAG);
                arm_node->mTransformation = matMat(Mat4::translate(obj.armature.base_pos));
                arm_node->mNumChildren = (unsigned int)obj.armature.roots.size();
                arm_node->mChildren = new aiNode*[obj.armature.roots.size()];

                std::unordered_map<Joint*, aiNode*> j_to_node;
                std::unordered_map<Skeleton::IK_Handle*, aiNode*> ik_to_node;

                std::function<void(aiNode*, Joint*)> joint_tree;
                joint_tree = [&](aiNode* node, Joint* j) {
                    std::string jname = jprefix + std::to_string(j->_id);
                    node->mName = aiString(jname);
                    j_to_node[j] = node;
                    size_t children = j->children.size();
                    for(Skeleton::IK_Handle* h : obj.armature.handles) {
                        if(h->joint == j) {
                            children++;
                        }
                    }
                    if(children == 0) return;

                    node->mNumChildren = (unsigned int)children;
                    node->mChildren = new aiNode*[children];
                    size_t i = 0;
                    for(Joint* c : j->children) {
                        node->mChildren[i] = new aiNode();
                        joint_tree(node->mChildren[i], c);
                        i++;
                    }
                    for(Skeleton::IK_Handle* h : obj.armature.handles) {
                        if(h->joint == j) {
                            aiNode* iknode = new aiNode();
                            std::string ikname = ikprefix + std::to_string(h->_id);
                            iknode->mName = aiString(ikname);
                            node->mChildren[i] = iknode;
                            ik_to_node[h] = iknode;
                            i++;
                        }
                    }
                };

                size_t i = 0;
                for(Joint* j : obj.armature.roots) {
                    aiNode* root_node = new aiNode();
                    arm_node->mChildren[i] = root_node;
                    j_to_node[j] = root_node;
                    joint_tree(root_node, j);
                    i++;
                }

                for(auto [j, n] : j_to_node) {
                    bone_nodes.insert({{obj.id(), j->_id}, n});
                }
                for(auto [h, n] : ik_to_node) {
                    bone_nodes.insert({{obj.id(), h->_id}, n});
                }

                obj.armature.for_joints([&](Joint* j) {
                    aiBone* bone = new aiBone();
                    ai_mesh->mBones[bone_idx++] = bone;
                    bone->mOffsetMatrix = matMat(Mat4::translate(j->extent) * Mat4::euler(j->pose));
                    bone->mNode = j_to_node[j];
                    std::string name = jprefix + std::to_string(j->_id);
                    bone->mName = aiString(name);
                    bone->mNumWeights = 1;
                    bone->mWeights = new aiVertexWeight[1];
                    bone->mWeights[0].mWeight = j->radius;
                });
                for(Skeleton::IK_Handle* h : obj.armature.handles) {
                    aiBone* bone = new aiBone();
                    ai_mesh->mBones[bone_idx++] = bone;
                    bone->mOffsetMatrix = matMat(Mat4::translate(h->target + obj.armature.base()));
                    bone->mNode = ik_to_node[h];
                    std::string ikname = ikprefix + std::to_string(h->_id);
                    bone->mName = aiString(ikname);
                    bone->mNumWeights = 1;
                    bone->mWeights = new aiVertexWeight[1];
                    bone->mWeights[0].mWeight = h->enabled ? 2.0f : 1.0f;
                }
            }

            if(obj.material.anim.splines.any()) {
                add_node(name + "-" + MAT_ANIM0);
                add_node(name + "-" + MAT_ANIM1);
            }

        } else if(entry.second.is<Scene_Particles>()) {

            const Scene_Particles& particles = entry.second.get<Scene_Particles>();

            std::string name(particles.opt.name);
            {
                std::replace(name.begin(), name.end(), ' ', '_');
                name += "-S3D-" + EMITTER_TAG + "-" + std::to_string(particles.id());
            }

            aiLight* ai_light = new aiLight();
            aiNode* ai_node = new aiNode();
            aiNode* ai_mesh_node = new aiNode();

            scene.mLights[light_idx++] = ai_light;
            scene.mRootNode->mChildren[node_idx++] = ai_node;
            scene.mRootNode->mChildren[node_idx++] = ai_mesh_node;

            write_particles(ai_light, particles.opt, name);

            Mat4 T = particles.pose.transform();
            item_nodes[particles.id()] = ai_node;
            ai_node->mName = aiString(name);
            ai_node->mTransformation = matMat(T);
            ai_node->mNumMeshes = 0;
            ai_node->mMeshes = nullptr;

            aiMesh* ai_mesh = new aiMesh();
            aiMaterial* ai_mat = new aiMaterial();
            size_t m_idx = mesh_idx++;
            scene.mMaterials[m_idx] = ai_mat;
            scene.mMeshes[m_idx] = ai_mesh;

            ai_mesh->mMaterialIndex = (unsigned int)m_idx;
            ai_mesh->mNumBones = 0;
            ai_mesh->mBones = nullptr;
            ai_mesh->mName = aiString(name + "-MESH");

            write_mesh(ai_mesh, particles.mesh());

            ai_mesh_node->mName = aiString(name + "-" + EMITTER_ANIM);
            ai_mesh_node->mNumMeshes = 1;
            ai_mesh_node->mMeshes = new unsigned int((unsigned int)m_idx);
            ai_mesh_node->mTransformation =
                matMat(Mat4::translate(Vec3{particles.opt.lifetime, 0.0f, 0.0f}));

        } else if(entry.second.is<Scene_Light>()) {

            const Scene_Light& light = entry.second.get<Scene_Light>();

            std::string name(light.opt.name);
            std::replace(name.begin(), name.end(), ' ', '_');

            aiLight* ai_light = new aiLight();
            aiNode* ai_node = new aiNode();

            scene.mLights[light_idx++] = ai_light;
            scene.mRootNode->mChildren[node_idx++] = ai_node;

            name = write_light(ai_light, light.opt, name, light.id(), light.emissive_loaded());

            Mat4 T = light.pose.transform();
            item_nodes[light.id()] = ai_node;
            ai_node->mName = aiString(name);
            ai_node->mNumMeshes = 0;
            ai_node->mMeshes = nullptr;
            ai_node->mTransformation = matMat(T);

            if(light.lanim.splines.any()) {
                aiNode* ai_node_light = new aiNode();
                scene.mRootNode->mChildren[node_idx++] = ai_node_light;
                ai_node_light->mName = aiString(name + "-" + LIGHT_ANIM);
                ai_node_light->mNumMeshes = 0;
                ai_node_light->mMeshes = nullptr;
                ai_node_light->mTransformation = aiMatrix4x4();
            }
        }
    }

    { // Animation data
        auto write_anim = [ai_anim = scene.mAnimations[0], &anim_idx](std::string name,
                                                                      auto splines, auto get_info) {
            if(!splines.any()) return;

            std::set<float> keys = splines.keys();
            unsigned int n_keys = (unsigned int)keys.size();

            aiNodeAnim* node = new aiNodeAnim();
            node->mNodeName = aiString(name);
            node->mNumPositionKeys = n_keys;
            node->mNumRotationKeys = n_keys;
            node->mNumScalingKeys = n_keys;
            node->mPositionKeys = new aiVectorKey[n_keys];
            node->mRotationKeys = new aiQuatKey[n_keys];
            node->mScalingKeys = new aiVectorKey[n_keys];

            ai_anim->mChannels[anim_idx++] = node;

            size_t i = 0;
            for(float k : keys) {
                auto [p, r, s] = get_info(k);
                node->mPositionKeys[i] = aiVectorKey(k, vecVec(p));
                node->mRotationKeys[i] = aiQuatKey(k, aiQuaternion(r.w, r.x, r.y, r.z));
                node->mScalingKeys[i] = aiVectorKey(k, vecVec(s));
                i++;
            }
        };

        const auto& anim_cam = animation.camera();
        write_anim(ANIM_CAM_NODE, anim_cam.splines,
                   [&anim_cam](float t) -> std::tuple<Vec3, Quat, Vec3> {
                       auto [p, r, fov, ar, ap, d] = anim_cam.splines.at(t);
                       (void)ar;
                       return {p, r, Vec3{fov, ap + 1.0f, d}};
                   });

        for(auto& entry : objs) {

            Scene_Item& item = entry.second;
            const Anim_Pose& pose = item.animation();
            aiNode* node = item_nodes[item.id()];

            write_anim(
                std::string(node->mName.C_Str()), pose.splines,
                [&pose](float t) -> std::tuple<Vec3, Quat, Vec3> { return pose.splines.at(t); });

            if(item.is<Scene_Object>()) {

                Skeleton& skel = item.get<Scene_Object>().armature;
                if(skel.has_keyframes()) {

                    skel.for_joints([&](Joint* j) {
                        aiNode* node = bone_nodes[{item.id(), j->_id}];
                        write_anim(std::string(node->mName.C_Str()), j->anim,
                                   [j](float t) -> std::tuple<Vec3, Quat, Vec3> {
                                       return {Vec3{0.0f}, j->anim.at(t), Vec3{1.0f}};
                                   });
                    });

                    skel.for_handles([&](Skeleton::IK_Handle* h) {
                        aiNode* node = bone_nodes[{item.id(), h->_id}];
                        write_anim(std::string(node->mName.C_Str()), h->anim,
                                   [h](float t) -> std::tuple<Vec3, Quat, Vec3> {
                                       auto [tr, e] = h->anim.at(t);
                                       return {tr, Quat{}, e ? Vec3{2.0f} : Vec3{1.0f}};
                                   });
                    });
                }
                {
                    std::string name0(node->mName.C_Str());
                    name0 += "-" + MAT_ANIM0;
                    std::string name1(node->mName.C_Str());
                    name1 += "-" + MAT_ANIM1;

                    const Material::Anim_Material& mat = item.get<Scene_Object>().material.anim;

                    write_anim(name0, mat.splines, [&mat](float t) -> std::tuple<Vec3, Quat, Vec3> {
                        auto [a, r, tr, e, in, ior] = mat.splines.at(t);
                        (void)tr;
                        (void)in;
                        (void)ior;
                        return {Vec3{a.r, a.g, a.b}, Quat::euler(Vec3{e.r, 0.0f, 0.0f}),
                                Vec3{r.r, r.g, r.b} + Vec3{1.0f}};
                    });
                    write_anim(name1, mat.splines, [&mat](float t) -> std::tuple<Vec3, Quat, Vec3> {
                        auto [a, r, tr, e, in, ior] = mat.splines.at(t);
                        (void)a;
                        (void)r;
                        return {Vec3{tr.r, tr.g, tr.b}, Quat::euler(Vec3{e.g, 0.0f, 0.0f}),
                                Vec3{in, ior, e.b} + Vec3{1.0f}};
                    });
                }

            } else if(item.is<Scene_Light>()) {

                std::string name(node->mName.C_Str());
                name += "-" + LIGHT_ANIM;

                const Scene_Light::Anim_Light& light = item.get<Scene_Light>().lanim;

                write_anim(name, light.splines, [&light](float t) -> std::tuple<Vec3, Quat, Vec3> {
                    auto [spec, inten, angle, size] = light.splines.at(t);
                    return {Vec3{spec.r, spec.g, spec.b}, Quat::euler(Vec3{angle.x, 0.0f, angle.y}),
                            Vec3{inten, size.x, size.y} + Vec3{1.0f}};
                });

            } else if(item.is<Scene_Particles>()) {

                std::string name(node->mName.C_Str());
                name += "-" + EMITTER_ANIM;

                const Scene_Particles::Anim_Particles& particles =
                    item.get<Scene_Particles>().panim;

                write_anim(
                    name, particles.splines, [&particles](float t) -> std::tuple<Vec3, Quat, Vec3> {
                        auto [col, vel, ang, scl, life, pps, en] = particles.splines.at(t);
                        return {Vec3{col.r, col.g, col.b}, Quat::euler(Vec3{scl, 0.0f, ang}),
                                Vec3{en ? (pps + 1.0f) : -(pps + 1.0f), life + 1.0f, vel + 1.0f}};
                    });
            }
        }
    }

    // Note: exporter/scene destructor will free everything
    Assimp::Exporter exporter;
    if(exporter.Export(&scene, "collada", file.c_str())) {
        return std::string(exporter.GetErrorString());
    }
    return {};
}
