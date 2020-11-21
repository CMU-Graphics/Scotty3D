
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
template <typename T1, typename T2> struct hash<pair<T1, T2>> {
    uint64_t operator()(const pair<T1, T2> &p) const {
        static const hash<T1> h1;
        static const hash<T2> h2;
        return h1(p.first) ^ h2(p.second);
    }
};
}; // namespace std

Scene_Item::Scene_Item(Scene_Object &&obj) : data(std::move(obj)) {}

Scene_Item::Scene_Item(Scene_Light &&light) : data(std::move(light)) {}

Scene_Item::Scene_Item(Scene_Item &&src) : data(std::move(src.data)) {}

Scene_Item &Scene_Item::operator=(Scene_Item &&src) {
    data = std::move(src.data);
    return *this;
}

void Scene_Item::set_time(float time) {
    return std::visit(overloaded{[time](Scene_Object &obj) { obj.set_time(time); },
                                 [time](Scene_Light &light) { light.set_time(time); }},
                      data);
}

BBox Scene_Item::bbox() {
    return std::visit(overloaded{[](Scene_Object &obj) { return obj.bbox(); },
                                 [](Scene_Light &light) { return light.bbox(); }},
                      data);
}

void Scene_Item::render(const Mat4 &view, bool solid, bool depth_only, bool posed) {
    std::visit(overloaded{[&](Scene_Object &obj) { obj.render(view, solid, depth_only, posed); },
                          [&](Scene_Light &light) { light.render(view, depth_only, posed); }},
               data);
}

Scene_ID Scene_Item::id() const {
    return std::visit(overloaded{[](const Scene_Object &obj) { return obj.id(); },
                                 [](const Scene_Light &light) { return light.id(); }},
                      data);
}

Anim_Pose &Scene_Item::animation() {

    Scene_Object *o = std::get_if<Scene_Object>(&data);
    if (o)
        return o->anim;
    Scene_Light *l = std::get_if<Scene_Light>(&data);
    if (l)
        return l->anim;

    assert(false);
    return l->anim;
}

const Anim_Pose &Scene_Item::animation() const {

    const Scene_Object *o = std::get_if<Scene_Object>(&data);
    if (o)
        return o->anim;
    const Scene_Light *l = std::get_if<Scene_Light>(&data);
    if (l)
        return l->anim;

    assert(false);
    return l->anim;
}

Pose &Scene_Item::pose() {

    Scene_Object *o = std::get_if<Scene_Object>(&data);
    if (o)
        return o->pose;
    Scene_Light *l = std::get_if<Scene_Light>(&data);
    if (l)
        return l->pose;

    assert(false);
    return l->pose;
}

const Pose &Scene_Item::pose() const {

    const Scene_Object *o = std::get_if<Scene_Object>(&data);
    if (o)
        return o->pose;
    const Scene_Light *l = std::get_if<Scene_Light>(&data);
    if (l)
        return l->pose;

    assert(false);
    return l->pose;
}

std::pair<char *, int> Scene_Item::name() {

    Scene_Object *o = std::get_if<Scene_Object>(&data);
    if (o)
        return {o->opt.name, Scene_Object::max_name_len};
    Scene_Light *l = std::get_if<Scene_Light>(&data);
    if (l)
        return {l->opt.name, Scene_Light::max_name_len};

    assert(false);
    return {l->opt.name, Scene_Object::max_name_len};
}

std::string Scene_Item::name() const {

    const Scene_Object *o = std::get_if<Scene_Object>(&data);
    if (o)
        return std::string(o->opt.name);
    const Scene_Light *l = std::get_if<Scene_Light>(&data);
    if (l)
        return std::string(l->opt.name);

    assert(false);
    return std::string(l->opt.name);
}

Scene::Scene(Scene_ID start) : next_id(start), first_id(start) {}

Scene_ID Scene::used_ids() { return next_id; }

Scene_ID Scene::reserve_id() { return next_id++; }

Scene_ID Scene::add(Pose pose, Halfedge_Mesh &&mesh, std::string n, Scene_ID id) {
    if (!id)
        id = next_id++;
    assert(objs.find(id) == objs.end());
    objs.emplace(std::make_pair(id, Scene_Object(id, pose, std::move(mesh), n)));
    return id;
}

Scene_ID Scene::add(Pose pose, GL::Mesh &&mesh, std::string n, Scene_ID id) {
    if (!id)
        id = next_id++;
    assert(objs.find(id) == objs.end());
    objs.emplace(std::make_pair(id, Scene_Object(id, pose, std::move(mesh), n)));
    return id;
}

std::string Scene::set_env_map(std::string file) {
    Scene_ID id = 0;
    for_items([&id](const Scene_Item &item) {
        if (item.is<Scene_Light>() && item.get<Scene_Light>().is_env())
            id = item.id();
    });
    Scene_Light l(Light_Type::sphere, reserve_id(), {}, "env_map");
    std::string err = l.emissive_load(file);
    if (err.empty()) {
        if (id)
            erase(id);
        add(std::move(l));
    }
    return err;
}

bool Scene::has_env_light() const {
    bool ret = false;
    for_items([&ret](const Scene_Item &item) {
        ret = ret || (item.is<Scene_Light>() && item.get<Scene_Light>().is_env());
    });
    return ret;
}

Scene_ID Scene::add(Scene_Light &&obj) {
    assert(objs.find(obj.id()) == objs.end());
    objs.emplace(std::make_pair(obj.id(), std::move(obj)));
    return obj.id();
}

Scene_ID Scene::add(Scene_Object &&obj) {
    assert(objs.find(obj.id()) == objs.end());
    objs.emplace(std::make_pair(obj.id(), std::move(obj)));
    return obj.id();
}

void Scene::restore(Scene_ID id) {
    if (objs.find(id) != objs.end())
        return;
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

void Scene::for_items(std::function<void(const Scene_Item &)> func) const {
    for (const auto &obj : objs) {
        func(obj.second);
    }
}

void Scene::for_items(std::function<void(Scene_Item &)> func) {
    for (auto &obj : objs) {
        func(obj.second);
    }
}

size_t Scene::size() { return objs.size(); }

bool Scene::empty() { return objs.size() == 0; }

Scene_Maybe Scene::get(Scene_ID id) {
    auto entry = objs.find(id);
    if (entry == objs.end())
        return std::nullopt;
    return entry->second;
}

Scene_Light &Scene::get_light(Scene_ID id) {
    auto entry = objs.find(id);
    assert(entry != objs.end());
    assert(entry->second.is<Scene_Light>());
    return entry->second.get<Scene_Light>();
}

Scene_Object &Scene::get_obj(Scene_ID id) {
    auto entry = objs.find(id);
    assert(entry != objs.end());
    assert(entry->second.is<Scene_Object>());
    return entry->second.get<Scene_Object>();
}

void Scene::clear(Undo &undo) {
    next_id = first_id;
    objs.clear();
    erased.clear();
    undo.reset();
}

static const std::string FAKE_NAME = "FAKE-S3D-FAKE-MESH";

static aiVector3D vecVec(Vec3 v) { return aiVector3D(v.x, v.y, v.z); }

static Quat aiQuat(aiQuaternion aiv) { return Quat(aiv.x, aiv.y, aiv.z, aiv.w); }

static Vec3 aiVec(aiVector3D aiv) { return Vec3(aiv.x, aiv.y, aiv.z); }

static Spectrum aiSpec(aiColor3D aiv) { return Spectrum(aiv.r, aiv.g, aiv.b); }

static aiMatrix4x4 matMat(const Mat4 &T) {
    return {T[0][0], T[1][0], T[2][0], T[3][0], T[0][1], T[1][1], T[2][1], T[3][1],
            T[0][2], T[1][2], T[2][2], T[3][2], T[0][3], T[1][3], T[2][3], T[3][3]};
}

static Mat4 aiMat(aiMatrix4x4 T) {
    return Mat4{Vec4{T[0][0], T[1][0], T[2][0], T[3][0]}, Vec4{T[0][1], T[1][1], T[2][1], T[3][1]},
                Vec4{T[0][2], T[1][2], T[2][2], T[3][2]}, Vec4{T[0][3], T[1][3], T[2][3], T[3][3]}};
}

static aiMatrix4x4 node_transform(const aiNode *node) {
    aiMatrix4x4 T;
    while (node) {
        T = T * node->mTransformation;
        node = node->mParent;
    }
    return T;
}

static void load_node(Scene &scobj, std::vector<std::string> &errors,
                      std::unordered_map<aiNode *, Scene_ID> &node_to_obj,
                      std::unordered_map<aiNode *, Joint *> &node_to_bone, const aiScene *scene,
                      aiNode *node, aiMatrix4x4 transform) {

    transform = transform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {

        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

        std::string name;
        bool do_flip = false, do_smooth = false;

        if (mesh->mName.length) {
            name = std::string(mesh->mName.C_Str());

            if (name.find(FAKE_NAME) != std::string::npos)
                continue;

            size_t special = name.find("-S3D-");
            if (special != std::string::npos) {
                if (name.find("FLIPPED") != std::string::npos)
                    do_flip = true;
                if (name.find("SMOOTHED") != std::string::npos)
                    do_smooth = true;
                name = name.substr(0, special);
                std::replace(name.begin(), name.end(), '_', ' ');
            }
        }

        std::vector<Vec3> verts;

        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            const aiVector3D &pos = mesh->mVertices[j];
            verts.push_back(Vec3(pos.x, pos.y, pos.z));
        }

        std::vector<std::vector<Halfedge_Mesh::Index>> polys;
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            const aiFace &face = mesh->mFaces[j];
            if(face.mNumIndices < 3) continue;
            std::vector<Halfedge_Mesh::Index> poly;
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                poly.push_back(face.mIndices[k]);
            }
            polys.push_back(poly);
        }

        aiVector3D ascale, arot, apos;
        transform.Decompose(ascale, arot, apos);
        Vec3 pos = aiVec(apos);
        Vec3 rot = aiVec(arot);
        Vec3 scale = aiVec(ascale);
        Pose p = {pos, Degrees(rot).range(0.0f, 360.0f), scale};

        Material::Options material;
        const aiMaterial &ai_mat = *scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D albedo;
        ai_mat.Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
        material.albedo = aiSpec(albedo);

        aiColor3D emissive;
        ai_mat.Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        material.emissive = aiSpec(emissive);

        aiColor3D reflectance;
        ai_mat.Get(AI_MATKEY_COLOR_REFLECTIVE, reflectance);
        material.reflectance = aiSpec(reflectance);

        aiColor3D transmittance;
        ai_mat.Get(AI_MATKEY_COLOR_TRANSPARENT, transmittance);
        material.transmittance = aiSpec(transmittance);

        ai_mat.Get(AI_MATKEY_REFRACTI, material.ior);
        ai_mat.Get(AI_MATKEY_SHININESS, material.intensity);

        aiString ai_type;
        ai_mat.Get(AI_MATKEY_NAME, ai_type);
        std::string type(ai_type.C_Str());

        if (type.find("lambertian") != std::string::npos) {
            material.type = Material_Type::lambertian;
        } else if (type.find("mirror") != std::string::npos) {
            material.type = Material_Type::mirror;
        } else if (type.find("refract") != std::string::npos) {
            material.type = Material_Type::refract;
        } else if (type.find("glass") != std::string::npos) {
            material.type = Material_Type::glass;
        } else if (type.find("diffuse_light") != std::string::npos) {
            material.type = Material_Type::diffuse_light;
        } else {
            material = Material::Options();
        }
        material.emissive *= 1.0f / material.intensity;

        Scene_Object new_obj;

        // Horrible hack
        if (type.find("SPHERESHAPE") != std::string::npos) {

            aiColor3D specular;
            ai_mat.Get(AI_MATKEY_COLOR_SPECULAR, specular);
            Scene_Object obj(scobj.reserve_id(), p, GL::Mesh());
            obj.material.opt = material;
            obj.opt.shape_type = PT::Shape_Type::sphere;
            obj.opt.shape = PT::Shape(PT::Sphere(specular.r));
            new_obj = std::move(obj);

        } else {

            Halfedge_Mesh hemesh;
            std::string err = hemesh.from_poly(polys, verts);
            if (!err.empty()) {

                std::vector<GL::Mesh::Vert> mesh_verts;
                std::vector<GL::Mesh::Index> mesh_inds;

                for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                    const aiVector3D &vpos = mesh->mVertices[j];
                    aiVector3D vnorm;
                    if(mesh->HasNormals()) {
                        vnorm = mesh->mNormals[j];
                    }
                    mesh_verts.push_back({aiVec(vpos), aiVec(vnorm), 0});
                }

                for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                    const aiFace &face = mesh->mFaces[j];
                    if(face.mNumIndices < 3) continue;
                    unsigned int start = face.mIndices[0];
                    for (size_t k = 1; k <= face.mNumIndices - 2; k++) {
                        mesh_inds.push_back(start);
                        mesh_inds.push_back(face.mIndices[k]);
                        mesh_inds.push_back(face.mIndices[k + 1]);
                    }
                }

                errors.push_back(err);
                Scene_Object obj(scobj.reserve_id(), p,
                                 GL::Mesh(std::move(mesh_verts), std::move(mesh_inds)), name);
                obj.material.opt = material;
                new_obj = std::move(obj);

            } else {

                if (do_flip)
                    hemesh.flip();
                Scene_Object obj(scobj.reserve_id(), p, std::move(hemesh), name);
                obj.material.opt = material;
                obj.opt.smooth_normals = do_smooth;
                new_obj = std::move(obj);
            }
        }

        if (mesh->mNumBones) {

            Skeleton &skeleton = new_obj.armature;
            aiNode *arm_node = mesh->mBones[0]->mArmature;
            if(arm_node) {
                {
                    aiVector3D t, r, s;
                    arm_node->mTransformation.Decompose(s, r, t);
                    skeleton.base() = aiVec(t);
                }

                std::unordered_map<aiNode *, aiBone *> node_to_aibone;
                for (unsigned int j = 0; j < mesh->mNumBones; j++) {
                    node_to_aibone[mesh->mBones[j]->mNode] = mesh->mBones[j];
                }

                std::function<void(Joint *, aiNode *)> build_tree;
                build_tree = [&](Joint *p, aiNode *node) {
                    aiBone *bone = node_to_aibone[node];
                    aiVector3D t, r, s;
                    bone->mOffsetMatrix.Decompose(s, r, t);

                    std::string name(bone->mName.C_Str());
                    if (name.find("S3D-joint-IK") != std::string::npos) {
                        Skeleton::IK_Handle *h = skeleton.add_handle(aiVec(t), p);
                        h->enabled = bone->mWeights[0].mWeight > 1.0f;
                    } else {
                        Joint *c = skeleton.add_child(p, aiVec(t));
                        node_to_bone[node] = c;
                        c->pose = aiVec(r);
                        c->radius = bone->mWeights[0].mWeight;
                        for (unsigned int j = 0; j < node->mNumChildren; j++)
                            build_tree(c, node->mChildren[j]);
                    }
                };
                for (unsigned int j = 0; j < arm_node->mNumChildren; j++) {
                    aiNode *root_node = arm_node->mChildren[j];
                    aiBone *root_bone = node_to_aibone[root_node];
                    aiVector3D t, r, s;
                    root_bone->mOffsetMatrix.Decompose(s, r, t);
                    Joint *root = skeleton.add_root(aiVec(t));
                    node_to_bone[root_node] = root;
                    root->pose = aiVec(r);
                    root->radius = root_bone->mWeights[0].mWeight;
                    for (unsigned int k = 0; k < root_node->mNumChildren; k++)
                        build_tree(root, root_node->mChildren[k]);
                }
            }
        }

        node_to_obj[node] = new_obj.id();
        scobj.add(std::move(new_obj));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        load_node(scobj, errors, node_to_obj, node_to_bone, scene, node->mChildren[i], transform);
    }
}

std::string Scene::load(bool new_scene, Undo &undo, Gui::Manager &gui, std::string file) {

    if (new_scene) {
        clear(undo);
        gui.get_animate().clear();
        gui.get_rig().clear();
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        file.c_str(), aiProcess_PopulateArmatureData | aiProcess_OptimizeMeshes |
                      aiProcess_ValidateDataStructure | aiProcess_FindInvalidData |
                      aiProcess_FindInstances | aiProcess_FindDegenerates |
                      aiProcess_DropNormals | aiProcess_JoinIdenticalVertices);

    if (!scene) {
        return "Parsing scene " + file + ": " + std::string(importer.GetErrorString());
    }

    std::vector<std::string> errors;
    std::unordered_map<aiNode *, Scene_ID> node_to_obj;
    std::unordered_map<aiNode *, Joint *> node_to_bone;
    scene->mRootNode->mTransformation = aiMatrix4x4();

    // Load objects
    load_node(*this, errors, node_to_obj, node_to_bone, scene, scene->mRootNode, aiMatrix4x4());

    // Load camera
    if (new_scene && scene->mNumCameras) {
        const aiCamera &aiCam = *scene->mCameras[0];
        Mat4 cam_transform = aiMat(node_transform(scene->mRootNode->FindNode(aiCam.mName)));
        Vec3 pos = cam_transform * aiVec(aiCam.mPosition);
        Vec3 center = cam_transform * aiVec(aiCam.mLookAt);
        gui.get_render().load_cam(pos, center, aiCam.mAspect, aiCam.mHorizontalFOV);
    }

    // Load lights
    for (unsigned int i = 0; i < scene->mNumLights; i++) {

        const aiLight &ailight = *scene->mLights[i];
        const aiNode *node = scene->mRootNode->FindNode(ailight.mName);

        Mat4 trans = aiMat(node_transform(node));
        Vec3 pos = trans * aiVec(ailight.mPosition);
        Vec3 dir = trans.rotate(Vec3{0.0f, 1.0f, 0.0f});

        Pose p;
        p.pos = pos;
        p.euler = Mat4::rotate_to(dir).to_euler();

        bool was_exported_from_s3d = false;
        float export_power = 1.0f;
        bool is_sphere = false, is_hemisphere = false, is_area = false;

        std::string name = std::string(node->mName.C_Str());

        size_t special = name.find("-S3D-");
        if (special != std::string::npos) {

            was_exported_from_s3d = true;
            export_power = ailight.mAttenuationQuadratic;

            std::string left = name.substr(special + 4);
            name = name.substr(0, special);
            std::replace(name.begin(), name.end(), '_', ' ');

            if (left.find("HEMISPHERE") != std::string::npos)
                is_hemisphere = true;
            else if (left.find("SPHERE") != std::string::npos)
                is_sphere = true;
            else if (left.find("AREA") != std::string::npos)
                is_area = true;
        }

        aiColor3D color;
        Scene_Light light(Light_Type::point, reserve_id(), p, name);

        switch (ailight.mType) {
        case aiLightSource_DIRECTIONAL: {
            light.opt.type = Light_Type::directional;
            color = ailight.mColorDiffuse;
        } break;
        case aiLightSource_POINT: {
            light.opt.type = Light_Type::point;
            color = ailight.mColorDiffuse;
        } break;
        case aiLightSource_SPOT: {
            light.opt.type = Light_Type::spot;
            light.opt.angle_bounds.x = ailight.mAngleInnerCone;
            light.opt.angle_bounds.y = ailight.mAngleOuterCone;
            color = ailight.mColorDiffuse;
        } break;
        case aiLightSource_AMBIENT: {
            if (is_hemisphere)
                light.opt.type = Light_Type::hemisphere;
            else if (is_sphere) {
                light.opt.type = Light_Type::sphere;
                if (ailight.mEnvMap.length) {
                    light.opt.has_emissive_map = true;
                    light.emissive_load(std::string(ailight.mEnvMap.C_Str()));
                }
            } else if (is_area) {
                light.opt.type = Light_Type::rectangle;
                light.opt.size.x = ailight.mAttenuationConstant;
                light.opt.size.y = ailight.mAttenuationLinear;
            }
            color = ailight.mColorAmbient;
        } break;
        case aiLightSource_AREA: {
            light.opt.type = Light_Type::rectangle;
            light.opt.size.x = ailight.mSize.x;
            light.opt.size.y = ailight.mSize.y;
            color = ailight.mColorDiffuse;
        } break;
        default:
            continue;
        }

        float power = std::max(color.r, std::max(color.g, color.b));
        light.opt.spectrum = Spectrum(color.r, color.g, color.b) * (1.0f / power);
        light.opt.intensity = power;

        if (was_exported_from_s3d) {
            float factor = power / export_power;
            light.opt.spectrum *= factor;
            light.opt.intensity = export_power;
        }

        if (!light.is_env() || !has_env_light()) {

            node_to_obj[(aiNode *)node] = light.id();

            std::string anim_name = std::string(node->mName.C_Str()) + "-LIGHT_ANIM_NODE";
            aiNode *anim_node = scene->mRootNode->FindNode(aiString(anim_name));
            node_to_obj[anim_node] = light.id();

            add(std::move(light));
        }
    }

    // animations
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {

        aiAnimation &anim = *scene->mAnimations[i];
        for (unsigned int j = 0; j < anim.mNumChannels; j++) {
            aiNodeAnim &node_anim = *anim.mChannels[j];

            if (node_anim.mNodeName == aiString("camera_node")) {

                Gui::Anim_Camera &cam = gui.get_animate().camera();
                unsigned int keys =
                    std::min(node_anim.mNumPositionKeys,
                             std::min(node_anim.mNumRotationKeys, node_anim.mNumScalingKeys));

                for (unsigned int k = 0; k < keys; k++) {
                    float t = (float)node_anim.mPositionKeys[k].mTime;
                    Vec3 pos = aiVec(node_anim.mPositionKeys[k].mValue);
                    Quat rot = aiQuat(node_anim.mRotationKeys[k].mValue);
                    Vec3 v = aiVec(node_anim.mScalingKeys[k].mValue);
                    cam.splines.set(t, pos, rot, v.x, v.y);
                }

            } else if (std::string(node_anim.mNodeName.C_Str()).find("LIGHT_ANIM_NODE") !=
                       std::string::npos) {

                aiNode *node = scene->mRootNode->FindNode(node_anim.mNodeName);
                auto entry = node_to_obj.find(node);
                if (entry != node_to_obj.end()) {

                    Scene_Light &l = get_light(entry->second);
                    Scene_Light::Anim_Light &light = l.lanim;
                    unsigned int keys =
                        std::min(node_anim.mNumPositionKeys,
                                 std::min(node_anim.mNumRotationKeys, node_anim.mNumScalingKeys));

                    for (unsigned int k = 0; k < keys; k++) {
                        float t = (float)node_anim.mPositionKeys[k].mTime;
                        Vec3 spec = aiVec(node_anim.mPositionKeys[k].mValue);
                        Vec3 angle = aiQuat(node_anim.mRotationKeys[k].mValue).to_euler();
                        Vec3 inten_sz = aiVec(node_anim.mScalingKeys[k].mValue);
                        light.splines.set(t, Spectrum{spec.x, spec.y, spec.z}, inten_sz.x - 1.0f,
                                          Vec2{angle.x, angle.z},
                                          Vec2{inten_sz.y - 1.0f, inten_sz.z - 1.0f});
                    }
                }

            } else {

                aiNode *node = scene->mRootNode->FindNode(node_anim.mNodeName);
                auto jentry = node_to_bone.find(node);
                if (jentry != node_to_bone.end()) {

                    Joint *jt = jentry->second;
                    unsigned int keys = node_anim.mNumRotationKeys;

                    for (unsigned int k = 0; k < keys; k++) {
                        float t = (float)node_anim.mRotationKeys[k].mTime;
                        Quat rot = aiQuat(node_anim.mRotationKeys[k].mValue);
                        jt->anim.set(t, rot);
                    }

                } else {
                    auto entry = node_to_obj.find(node);
                    if (entry != node_to_obj.end()) {

                        Scene_Item &item = *get(entry->second);
                        Anim_Pose &pose = item.animation();
                        unsigned int keys = std::min(
                            node_anim.mNumPositionKeys,
                            std::min(node_anim.mNumRotationKeys, node_anim.mNumScalingKeys));

                        for (unsigned int k = 0; k < keys; k++) {
                            float t = (float)node_anim.mPositionKeys[k].mTime;
                            Vec3 pos = aiVec(node_anim.mPositionKeys[k].mValue);
                            Quat rot = aiQuat(node_anim.mRotationKeys[k].mValue);
                            Vec3 scl = aiVec(node_anim.mScalingKeys[k].mValue);
                            pose.splines.set(t, pos, rot, scl);
                        }
                    }
                }
            }
        }

        if (anim.mDuration > 0.0f) {
            gui.get_animate().set((int)std::ceil(anim.mDuration),
                                  (int)std::round(anim.mTicksPerSecond));
        }
        gui.get_animate().refresh(*this);
    }

    std::stringstream stream;
    if (errors.size()) {
        stream << "Meshes with errors may not be edit-able in the model mode." << std::endl
               << std::endl;
    }
    for (size_t i = 0; i < errors.size(); i++) {
        stream << "Loading mesh " << i << ": " << errors[i] << std::endl;
    }
    return stream.str();
}

std::string Scene::write(std::string file, const Camera &render_cam,
                         const Gui::Animate &animation) {

    bool no_real_meshes = false;

    size_t n_meshes = 0, n_lights = 0, n_anims = 0;
    size_t n_armatures = 0;
    for_items([&](Scene_Item &item) {
        if (item.is<Scene_Object>()) {
            Scene_Object &obj = item.get<Scene_Object>();
            n_meshes++;
            if (obj.anim.splines.any())
                n_anims++;
            if (obj.armature.has_bones())
                n_armatures++;
            obj.armature.for_joints([&](Joint *j) {
                if (j->anim.any())
                    n_anims++;
            });
        } else if (item.is<Scene_Light>()) {
            if (item.animation().splines.any())
                n_anims++;
            if (item.get<Scene_Light>().lanim.splines.any())
                n_anims++;
            n_lights++;
        }
    });

    size_t camera_anim = n_anims;
    if (animation.camera().splines.any())
        n_anims++;

    if (!n_meshes) {
        no_real_meshes = true;
        n_meshes = 1;
    }

    aiScene scene;
    scene.mRootNode = new aiNode();

    // materials
    scene.mMaterials = new aiMaterial *[n_meshes]();
    scene.mNumMaterials = (unsigned int)n_meshes;

    // camera
    const Gui::Anim_Camera &anim_cam = animation.camera();
    Camera cam = render_cam;
    if (anim_cam.splines.any()) {
        cam = animation.camera().at(0.0f);
    }
    scene.mCameras = new aiCamera *[1]();
    scene.mCameras[0] = new aiCamera();
    scene.mCameras[0]->mAspect = cam.get_ar();
    scene.mCameras[0]->mClipPlaneNear = cam.get_near();
    scene.mCameras[0]->mClipPlaneFar = 100.0f;
    scene.mCameras[0]->mLookAt = aiVector3D(0.0f, 0.0f, -1.0f);
    scene.mCameras[0]->mHorizontalFOV = Radians(cam.get_h_fov());
    scene.mCameras[0]->mName = aiString("camera_node");
    scene.mNumCameras = 1;

    // nodes
    size_t cam_idx = n_meshes + n_armatures + n_lights * 2;
    size_t n_nodes = cam_idx + 1;

    scene.mRootNode->mNumMeshes = 0;
    scene.mRootNode->mNumChildren = (unsigned int)(n_nodes);
    scene.mRootNode->mChildren = new aiNode *[n_nodes]();

    // camera node
    Mat4 view = cam.get_view().inverse();
    scene.mRootNode->mChildren[cam_idx] = new aiNode();
    scene.mRootNode->mChildren[cam_idx]->mNumMeshes = 0;
    scene.mRootNode->mChildren[cam_idx]->mName = aiString("camera_node");
    scene.mRootNode->mChildren[cam_idx]->mTransformation = matMat(view);

    // meshes
    scene.mMeshes = new aiMesh *[n_meshes]();
    scene.mNumMeshes = (unsigned int)n_meshes;

    // lights
    scene.mLights = new aiLight *[n_lights]();
    scene.mNumLights = (unsigned int)n_lights;

    size_t mesh_idx = 0, light_idx = 0, node_idx = 0;

    if (no_real_meshes) {
        aiMesh *ai_mesh = new aiMesh();
        aiNode *ai_node = new aiNode();
        scene.mMeshes[mesh_idx++] = ai_mesh;
        scene.mRootNode->mChildren[node_idx++] = ai_node;
        ai_node->mNumMeshes = 1;
        ai_node->mMeshes = new unsigned int((unsigned int)0);

        ai_mesh->mVertices = new aiVector3D[3];
        ai_mesh->mNormals = new aiVector3D[3];
        ai_mesh->mNumVertices = (unsigned int)3;
        ai_mesh->mVertices[0] = vecVec(Vec3{1.0f, 0.0f, 0.0f});
        ai_mesh->mVertices[1] = vecVec(Vec3{0.0f, 1.0f, 0.0f});
        ai_mesh->mVertices[2] = vecVec(Vec3{0.0f, 0.0f, 1.0f});
        ai_mesh->mNormals[0] = vecVec(Vec3{0.0f, 1.0f, 0.0f});
        ai_mesh->mNormals[1] = vecVec(Vec3{0.0f, 1.0f, 0.0f});
        ai_mesh->mNormals[2] = vecVec(Vec3{0.0f, 1.0f, 0.0f});

        ai_mesh->mFaces = new aiFace[1];
        ai_mesh->mNumFaces = 1u;

        ai_mesh->mFaces[0].mIndices = new unsigned int[3];
        ai_mesh->mFaces[0].mNumIndices = 3;
        ai_mesh->mFaces[0].mIndices[0] = 0;
        ai_mesh->mFaces[0].mIndices[1] = 1;
        ai_mesh->mFaces[0].mIndices[2] = 2;

        ai_mesh->mName = aiString(FAKE_NAME);
        ai_node->mName = aiString(FAKE_NAME);
        scene.mMaterials[0] = new aiMaterial();
    }

    std::unordered_map<Scene_ID, aiNode *> item_nodes;
    std::unordered_map<std::pair<Scene_ID, Scene_ID>, aiNode *> bone_nodes;

    for (auto &entry : objs) {

        if (entry.second.is<Scene_Object>()) {

            Scene_Object &obj = entry.second.get<Scene_Object>();

            if (obj.is_shape()) {
                obj.try_make_editable(obj.opt.shape_type);
            }

            aiMesh *ai_mesh = new aiMesh();
            aiNode *ai_node = new aiNode();
            aiMaterial *ai_mat = new aiMaterial();

            size_t idx = mesh_idx++;
            scene.mMaterials[idx] = ai_mat;
            scene.mMeshes[idx] = ai_mesh;
            scene.mRootNode->mChildren[node_idx++] = ai_node;

            ai_mesh->mMaterialIndex = (unsigned int)idx;
            ai_node->mNumMeshes = 1;
            ai_node->mMeshes = new unsigned int((unsigned int)idx);

            if (obj.is_editable()) {

                Halfedge_Mesh &mesh = obj.get_mesh();
                size_t n_verts = mesh.n_vertices();
                size_t n_faces = mesh.n_faces();

                ai_mesh->mVertices = new aiVector3D[n_verts];
                ai_mesh->mNumVertices = (unsigned int)n_verts;

                ai_mesh->mFaces = new aiFace[n_faces];
                ai_mesh->mNumFaces = (unsigned int)(n_faces);

                std::unordered_map<size_t, size_t> id_to_idx;

                size_t vert_idx = 0;
                for (auto v = mesh.vertices_begin(); v != mesh.vertices_end(); v++) {
                    id_to_idx[v->id()] = vert_idx;
                    ai_mesh->mVertices[vert_idx] = vecVec(v->pos);
                    vert_idx++;
                }

                size_t face_idx = 0;
                for (auto f = mesh.faces_begin(); f != mesh.faces_end(); f++) {

                    aiFace &face = ai_mesh->mFaces[face_idx];
                    face.mIndices = new unsigned int[f->degree()];
                    face.mNumIndices = f->degree();

                    size_t i_idx = 0;
                    auto h = f->halfedge();
                    do {
                        face.mIndices[i_idx] = (unsigned int)id_to_idx[h->vertex()->id()];
                        h = h->next();
                        i_idx++;
                    } while (h != f->halfedge());

                    face_idx++;
                }

            } else {

                const auto &verts = obj.mesh().verts();
                const auto &elems = obj.mesh().indices();

                ai_mesh->mVertices = new aiVector3D[verts.size()];
                ai_mesh->mNormals = new aiVector3D[verts.size()];
                ai_mesh->mNumVertices = (unsigned int)verts.size();

                int j = 0;
                for (GL::Mesh::Vert v : verts) {
                    ai_mesh->mVertices[j] = vecVec(v.pos);
                    ai_mesh->mNormals[j] = vecVec(v.norm);
                    j++;
                }

                ai_mesh->mFaces = new aiFace[elems.size() / 3];
                ai_mesh->mNumFaces = (unsigned int)(elems.size() / 3);

                for (size_t i = 0; i < elems.size(); i += 3) {
                    aiFace &face = ai_mesh->mFaces[i / 3];
                    face.mIndices = new unsigned int[3];
                    face.mNumIndices = 3;
                    face.mIndices[0] = elems[i];
                    face.mIndices[1] = elems[i + 1];
                    face.mIndices[2] = elems[i + 2];
                }

            }

            std::string name(obj.opt.name);
            std::replace(name.begin(), name.end(), ' ', '_');
            name += "-S3D-" + std::to_string(obj.id());

            if (obj.get_mesh().flipped())
                name += "-FLIPPED";
            if (obj.opt.smooth_normals)
                name += "-SMOOTHED";

            ai_mesh->mName = aiString(name);

            Mat4 trans = obj.pose.transform();

            item_nodes[obj.id()] = ai_node;
            ai_node->mName = aiString(name);
            ai_node->mTransformation = matMat(trans);

            const Material::Options &opt = obj.material.opt;
            std::string mat_name;

            switch (opt.type) {
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
            default:
                break;
            }

            // Horrible hack
            if (obj.opt.shape_type == PT::Shape_Type::sphere) {
                mat_name += "-SPHERESHAPE";
                ai_mat->AddProperty(new aiColor3D(obj.opt.shape.get<PT::Sphere>().radius), 1,
                                    AI_MATKEY_COLOR_SPECULAR);
            }

            Spectrum emissive = obj.material.emissive();

            // diffuse -> albedo
            // reflective -> reflectance
            // transparent -> transmittance
            // emissive -> emissive
            // refracti -> index of refraction
            // shininess -> intensity
            ai_mat->AddProperty(new aiString(mat_name), AI_MATKEY_NAME);
            ai_mat->AddProperty(new aiColor3D(opt.albedo.r, opt.albedo.g, opt.albedo.b), 1,
                                AI_MATKEY_COLOR_DIFFUSE);
            ai_mat->AddProperty(
                new aiColor3D(opt.reflectance.r, opt.reflectance.g, opt.reflectance.b), 1,
                AI_MATKEY_COLOR_REFLECTIVE);
            ai_mat->AddProperty(
                new aiColor3D(opt.transmittance.r, opt.transmittance.g, opt.transmittance.b), 1,
                AI_MATKEY_COLOR_TRANSPARENT);
            ai_mat->AddProperty(new aiColor3D(emissive.r, emissive.g, emissive.b), 1,
                                AI_MATKEY_COLOR_EMISSIVE);
            ai_mat->AddProperty(new float(opt.ior), 1, AI_MATKEY_REFRACTI);
            ai_mat->AddProperty(new float(opt.intensity), 1, AI_MATKEY_SHININESS);

            if (obj.armature.has_bones()) {
                ai_mesh->mNumBones = obj.armature.n_bones() + obj.armature.n_handles();
                ai_mesh->mBones = new aiBone *[ai_mesh->mNumBones];

                size_t bone_idx = 0;
                std::string jprefix = "S3D-joint-" + std::to_string(obj.id()) + "-";
                std::string ikprefix = "S3D-joint-IK-" + std::to_string(obj.id()) + "-";

                aiNode *arm_node = new aiNode();
                scene.mRootNode->mChildren[node_idx++] = arm_node;
                arm_node->mName = aiString(jprefix + "armature");
                arm_node->mTransformation = matMat(Mat4::translate(obj.armature.base_pos));
                arm_node->mNumChildren = (unsigned int)obj.armature.roots.size();
                arm_node->mChildren = new aiNode *[obj.armature.roots.size()];

                std::unordered_map<Joint *, aiNode *> j_to_node;
                std::unordered_map<Skeleton::IK_Handle *, aiNode *> ik_to_node;

                std::function<void(aiNode *, Joint *)> joint_tree;
                joint_tree = [&](aiNode *node, Joint *j) {
                    std::string jname = jprefix + std::to_string(j->_id);
                    node->mName = aiString(jname);
                    j_to_node[j] = node;
                    size_t children = j->children.size();
                    for (Skeleton::IK_Handle* h : obj.armature.handles) {
                        if (h->joint == j) {
                            children++;
                        }
                    }
                    if (children > 0) {
                        node->mNumChildren = (unsigned int)children;
                        node->mChildren = new aiNode *[children];
                        size_t i = 0;
                        for (Joint *c : j->children) {
                            node->mChildren[i] = new aiNode();
                            joint_tree(node->mChildren[i], c);
                            i++;
                        }
                        for (Skeleton::IK_Handle* h : obj.armature.handles) {
                            if (h->joint == j) {
                                aiNode* iknode = new aiNode();
                                std::string ikname = ikprefix + std::to_string(h->_id);
                                iknode->mName = aiString(ikname);
                                node->mChildren[i] = iknode;
                                ik_to_node[h] = iknode;
                                i++;
                            }
                        }
                    }
                };

                size_t i = 0;
                for (Joint *j : obj.armature.roots) {
                    aiNode *root_node = new aiNode();
                    arm_node->mChildren[i] = root_node;
                    j_to_node[j] = root_node;
                    joint_tree(root_node, j);
                    i++;
                }

                for (auto [j, n] : j_to_node) {
                    bone_nodes.insert({{obj.id(), j->_id}, n});
                }

                obj.armature.for_joints([&](Joint *j) {
                    aiBone *bone = new aiBone();
                    ai_mesh->mBones[bone_idx++] = bone;
                    bone->mOffsetMatrix = matMat(Mat4::translate(j->extent) * Mat4::euler(j->pose));
                    bone->mNode = j_to_node[j];
                    std::string name = jprefix + std::to_string(j->_id);
                    bone->mName = aiString(name);
                    bone->mNumWeights = 1;
                    bone->mWeights = new aiVertexWeight[1];
                    bone->mWeights[0].mWeight = j->radius;
                });
                for (Skeleton::IK_Handle* h : obj.armature.handles) {
                    aiBone *bone = new aiBone();
                    ai_mesh->mBones[bone_idx++] = bone;
                    bone->mOffsetMatrix = matMat(Mat4::translate(h->target));
                    bone->mNode = ik_to_node[h];
                    std::string ikname = ikprefix + std::to_string(h->_id);
                    bone->mName = aiString(ikname);
                    bone->mNumWeights = 1;
                    bone->mWeights = new aiVertexWeight[1];
                    bone->mWeights[0].mWeight = h->enabled ? 2.0f : 1.0f;
                }
            }

        } else if (entry.second.is<Scene_Light>()) {

            const Scene_Light &light = entry.second.get<Scene_Light>();

            std::string name(light.opt.name);
            std::replace(name.begin(), name.end(), ' ', '_');

            aiLight *ai_light = new aiLight();
            aiNode *ai_node = new aiNode();
            aiNode *ai_node_light = new aiNode();

            scene.mLights[light_idx++] = ai_light;
            scene.mRootNode->mChildren[node_idx++] = ai_node;
            scene.mRootNode->mChildren[node_idx++] = ai_node_light;

            switch (light.opt.type) {
            case Light_Type::directional: {
                ai_light->mType = aiLightSource_DIRECTIONAL;
                name += "-S3D-" + std::to_string(light.id());
            } break;
            case Light_Type::sphere: {
                ai_light->mType = aiLightSource_AMBIENT;
                name += "-S3D-SPHERE-" + std::to_string(light.id());
                if (light.opt.has_emissive_map) {
                    ai_light->mEnvMap = aiString(light.emissive_loaded());
                }
            } break;
            case Light_Type::hemisphere: {
                ai_light->mType = aiLightSource_AMBIENT;
                name += "-S3D-HEMISPHERE-" + std::to_string(light.id());
            } break;
            case Light_Type::point: {
                ai_light->mType = aiLightSource_POINT;
                name += "-S3D-" + std::to_string(light.id());
            } break;
            case Light_Type::spot: {
                ai_light->mType = aiLightSource_SPOT;
                name += "-S3D-" + std::to_string(light.id());
            } break;
            case Light_Type::rectangle: {
                // the collada exporter literally just ignores area lights ????????
                ai_light->mType = aiLightSource_AMBIENT;
                name += "-S3D-AREA-" + std::to_string(light.id());
                ai_light->mAttenuationConstant = light.opt.size.x;
                ai_light->mAttenuationLinear = light.opt.size.y;
            } break;
            default:
                break;
            }

            Spectrum r = light.radiance();

            ai_light->mName = aiString(name);
            ai_light->mPosition = aiVector3D(0.0f, 0.0f, 0.0f);
            ai_light->mDirection = aiVector3D(0.0f, 1.0f, 0.0f);
            ai_light->mUp = aiVector3D(0.0f, 1.0f, 0.0f);
            ai_light->mSize = aiVector2D(light.opt.size.x, light.opt.size.y);
            ai_light->mAngleInnerCone = light.opt.angle_bounds.x;
            ai_light->mAngleOuterCone = light.opt.angle_bounds.y;
            ai_light->mColorDiffuse = aiColor3D(r.r, r.g, r.b);
            ai_light->mColorAmbient = aiColor3D(r.r, r.g, r.b);
            ai_light->mColorDiffuse = aiColor3D(r.r, r.g, r.b);
            ai_light->mAttenuationQuadratic = light.opt.intensity;

            Mat4 T = light.pose.transform();
            item_nodes[light.id()] = ai_node;
            ai_node->mName = aiString(name);
            ai_node->mNumMeshes = 0;
            ai_node->mMeshes = nullptr;
            ai_node->mTransformation = matMat(T);

            ai_node_light->mName = aiString(name + "-LIGHT_ANIM_NODE");
            ai_node_light->mNumMeshes = 0;
            ai_node_light->mMeshes = nullptr;
            ai_node_light->mTransformation = aiMatrix4x4();
        }
    }
    {
        scene.mNumAnimations = 1;
        scene.mAnimations = new aiAnimation *[1];
        scene.mAnimations[0] = new aiAnimation();
        aiAnimation &ai_anim = *scene.mAnimations[0];

        ai_anim.mName = aiString("Scotty3D-animate");
        ai_anim.mDuration = (double)animation.n_frames();
        ai_anim.mTicksPerSecond = (double)animation.fps();
        ai_anim.mNumChannels = (unsigned int)n_anims;
        ai_anim.mChannels = new aiNodeAnim *[n_anims];

        auto write_keyframes = [](aiNodeAnim *node, std::string name, auto pt, auto rot, auto scl) {
            node->mNodeName = aiString(name);
            node->mNumPositionKeys = (unsigned int)pt.size();
            node->mNumRotationKeys = (unsigned int)rot.size();
            node->mNumScalingKeys = (unsigned int)scl.size();
            node->mPositionKeys = new aiVectorKey[pt.size()];
            node->mRotationKeys = new aiQuatKey[rot.size()];
            node->mScalingKeys = new aiVectorKey[scl.size()];
            size_t i = 0;
            for (auto &e : pt) {
                node->mPositionKeys[i] = aiVectorKey(e.first, vecVec(e.second));
                i++;
            }
            i = 0;
            for (auto &e : rot) {
                node->mRotationKeys[i] = aiQuatKey(
                    e.first, aiQuaternion(e.second.w, e.second.x, e.second.y, e.second.z));
                i++;
            }
            i = 0;
            for (auto &e : scl) {
                node->mScalingKeys[i] = aiVectorKey(e.first, vecVec(e.second));
                i++;
            }
        };

        if (anim_cam.splines.any()) {
            ai_anim.mChannels[camera_anim] = new aiNodeAnim();

            std::set<float> keys = anim_cam.splines.keys();
            std::unordered_map<float, Vec3> points, scales;
            std::unordered_map<float, Quat> rotations;
            for (float k : keys) {
                auto [p, r, fov, ar] = anim_cam.splines.at(k);
                points[k] = p;
                rotations[k] = r;
                scales[k] = Vec3{fov, ar, 1.0f};
            }
            write_keyframes(ai_anim.mChannels[camera_anim], "camera_node", points, rotations,
                            scales);
        }

        size_t anim_idx = 0;
        for (auto &entry : objs) {

            Scene_Item &item = entry.second;
            const Anim_Pose &pose = item.animation();

            if (pose.splines.any()) {
                std::set<float> keys = pose.splines.keys();
                std::unordered_map<float, Vec3> points, scales;
                std::unordered_map<float, Quat> rotations;
                for (float k : keys) {
                    auto [p, r, s] = pose.splines.at(k);
                    points[k] = p;
                    rotations[k] = r;
                    scales[k] = s;
                }

                aiNode *node = item_nodes[item.id()];
                ai_anim.mChannels[anim_idx] = new aiNodeAnim();
                write_keyframes(ai_anim.mChannels[anim_idx], std::string(node->mName.C_Str()),
                                points, rotations, scales);
                anim_idx++;
            }

            if (item.is<Scene_Object>()) {

                Skeleton &skel = item.get<Scene_Object>().armature;
                if (skel.has_keyframes()) {

                    skel.for_joints([&](Joint *j) {
                        std::set<float> keys = j->anim.keys();
                        std::unordered_map<float, Vec3> points, scales;
                        std::unordered_map<float, Quat> rotations;

                        for (float k : keys) {
                            points[k] = Vec3{0.0f};
                            rotations[k] = j->anim.at(k);
                            scales[k] = Vec3{1.0f};
                        }

                        aiNode *node = bone_nodes[{item.id(), j->_id}];
                        ai_anim.mChannels[anim_idx] = new aiNodeAnim();
                        write_keyframes(ai_anim.mChannels[anim_idx],
                                        std::string(node->mName.C_Str()), points, rotations,
                                        scales);
                        anim_idx++;
                    });
                }

            } else if (item.is<Scene_Light>()) {

                aiNode *node = item_nodes[item.id()];
                std::string name(node->mName.C_Str());
                name += "-LIGHT_ANIM_NODE";

                const Scene_Light::Anim_Light &light = item.get<Scene_Light>().lanim;

                if (light.splines.any()) {
                    std::unordered_map<float, Vec3> points, scales;
                    std::unordered_map<float, Quat> rotations;
                    std::set<float> keys = light.splines.keys();
                    for (float k : keys) {
                        auto [spec, inten, angle, size] = light.splines.at(k);
                        points[k] = Vec3{spec.r, spec.g, spec.b};
                        rotations[k] = Quat::euler(Vec3{angle.x, 0.0f, angle.y});
                        scales[k] = Vec3{inten + 1.0f, size.x + 1.0f, size.y + 1.0f};
                    }

                    ai_anim.mChannels[anim_idx] = new aiNodeAnim();
                    write_keyframes(ai_anim.mChannels[anim_idx], name, points, rotations, scales);
                    anim_idx++;
                }
            }
        }
    }

    // Note: exporter/scene destructor should free everything

    Assimp::Exporter exporter;
    if (exporter.Export(&scene, "collada", file.c_str())) {
        return std::string(exporter.GetErrorString());
    }
    return {};
}
