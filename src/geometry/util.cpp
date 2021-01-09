
#include "util.h"

#include <map>

namespace Util {

GL::Mesh cyl_mesh(float radius, float height, int sides, bool cap) {
    return cone_mesh(radius, radius, height, sides, cap);
}

GL::Mesh arrow_mesh(float rbase, float rtip, float height) {
    Gen::Data base = Gen::cone(rbase, rbase, 0.75f * height, 10, true);
    Gen::Data tip = Gen::cone(rtip, 0.001f, 0.25f * height, 10, true);
    for(auto& v : tip.verts) v.pos.y += 0.7f;
    return Gen::merge(std::move(base), std::move(tip));
}

GL::Mesh scale_mesh() {
    Gen::Data base = Gen::cone(0.03f, 0.03f, 0.7f, 10, true);
    Gen::Data tip = Gen::cube(0.1f);
    for(auto& v : tip.verts) v.pos.y += 0.7f;
    return Gen::merge(std::move(base), std::move(tip));
}

GL::Mesh cone_mesh(float bradius, float tradius, float height, int sides, bool cap) {
    Gen::Data cone = Gen::cone(bradius, tradius, height, sides, cap);
    return Gen::dedup({std::move(cone.verts), std::move(cone.elems)});
}

GL::Mesh torus_mesh(float iradius, float oradius, int segments, int sides) {
    Gen::Data torus = Gen::torus(iradius, oradius, segments, sides);
    return Gen::dedup({std::move(torus.verts), std::move(torus.elems)});
}

GL::Mesh cube_mesh(float r) {
    Gen::Data cube = Gen::cube(r);
    return GL::Mesh(std::move(cube.verts), std::move(cube.elems));
}

GL::Mesh square_mesh(float r) {
    Gen::Data square = Gen::quad(r, r);
    return GL::Mesh(std::move(square.verts), std::move(square.elems));
}

GL::Mesh quad_mesh(float x, float y) {
    Gen::Data square = Gen::quad(x, y);
    return GL::Mesh(std::move(square.verts), std::move(square.elems));
}

GL::Mesh sphere_mesh(float r, int i) {
    Gen::Data ico_sphere = Gen::ico_sphere(r, i);
    return GL::Mesh(std::move(ico_sphere.verts), std::move(ico_sphere.elems));
}

GL::Mesh hemi_mesh(float r) {
    Gen::Data hemi = Gen::uv_hemisphere(r);
    return GL::Mesh(std::move(hemi.verts), std::move(hemi.elems));
}

GL::Mesh capsule_mesh(float h, float r) {

    Gen::Data bottom = Gen::uv_hemisphere(r);
    Gen::Data top = Gen::uv_hemisphere(r);
    for(auto& v : top.verts) v.pos.y = -v.pos.y + h;
    Gen::Data cyl = Gen::cone(r, r, h, 64, false);

    GL::Mesh::Index cyl_off = (GL::Mesh::Index)bottom.verts.size();
    GL::Mesh::Index top_off = cyl_off + (GL::Mesh::Index)cyl.verts.size();

    for(auto& i : cyl.elems) i += cyl_off;
    for(auto& i : top.elems) i += top_off;

    bottom.verts.insert(bottom.verts.end(), cyl.verts.begin(), cyl.verts.end());
    bottom.elems.insert(bottom.elems.end(), cyl.elems.begin(), cyl.elems.end());

    bottom.verts.insert(bottom.verts.end(), top.verts.begin(), top.verts.end());
    bottom.elems.insert(bottom.elems.end(), top.elems.begin(), top.elems.end());

    return GL::Mesh(std::move(bottom.verts), std::move(bottom.elems));
}

GL::Lines spotlight_mesh(Vec3 color, float inner, float outer) {

    const int steps = 72;
    const float step = (2.0f * PI_F) / (steps + 1);
    const float dist = 5.0f;

    inner = clamp(inner / 2.0f, 0.0f, 90.0f);
    outer = clamp(outer / 2.0f, 0.0f, 90.0f);
    float ri = dist * std::tan(Radians(inner));
    float ro = dist * std::tan(Radians(outer));
    Gen::LData iring = Gen::circle(color, ri, steps);
    Gen::LData oring = Gen::circle(color, ro, steps);
    Gen::LData rings = Gen::merge(std::move(iring), std::move(oring));
    for(auto& v : rings.verts) v.pos.y += 5.0f;

    float t = 0.0f;
    for(int i = 0; i < steps; i += steps / 4) {
        Vec3 point = ro * Vec3(std::sin(t), 0.0f, std::cos(t));
        rings.verts.push_back({{}, color});
        rings.verts.push_back({Vec3(point.x, 5.0f, point.z), color});
        t += step * (steps / 4);
    }
    return GL::Lines(std::move(rings.verts), 1.0f);
}

namespace Gen {

GL::Mesh dedup(Data&& d) {

    std::vector<GL::Mesh::Vert> verts;
    std::vector<GL::Mesh::Index> elems;

    // normals be damned
    std::map<Vec3, GL::Mesh::Index> v_to_idx;

    for(size_t i = 0; i < d.elems.size(); i++) {
        GL::Mesh::Index idx = d.elems[i];
        GL::Mesh::Vert v = d.verts[idx];
        auto entry = v_to_idx.find(v.pos);
        GL::Mesh::Index new_idx;
        if(entry == v_to_idx.end()) {
            new_idx = (GL::Mesh::Index)verts.size();
            v_to_idx.insert({v.pos, new_idx});
            verts.push_back(v);
        } else {
            new_idx = entry->second;
        }
        elems.push_back(new_idx);
    }

    return GL::Mesh(std::move(verts), std::move(elems));
}

GL::Mesh merge(Data&& l, Data&& r) {
    for(auto& i : r.elems) i += (GL::Mesh::Index)l.verts.size();
    l.verts.insert(l.verts.end(), r.verts.begin(), r.verts.end());
    l.elems.insert(l.elems.end(), r.elems.begin(), r.elems.end());
    return GL::Mesh(std::move(l.verts), std::move(l.elems));
}

LData merge(LData&& l, LData&& r) {
    l.verts.insert(l.verts.end(), r.verts.begin(), r.verts.end());
    return std::move(l);
}

LData circle(Vec3 color, float r, int sides) {

    std::vector<Vec3> points;
    float t = 0.0f;
    float step = (2.0f * PI_F) / (sides + 1);
    for(int i = 0; i < sides; i++) {
        points.push_back(r * Vec3(std::sin(t), 0.0f, std::cos(t)));
        t += step;
    }

    std::vector<GL::Lines::Vert> verts;
    for(size_t i = 0; i < points.size(); i++) {
        verts.push_back({points[i], color});
        verts.push_back({points[(i + 1) % points.size()], color});
    }

    return LData{std::move(verts)};
}

Data quad(float x, float y) {
    return {{{Vec3{-x, 0.0f, -y}, Vec3{0.0f, 1.0f, 0.0f}, 0},
             {Vec3{-x, 0.0f, y}, Vec3{0.0f, 1.0f, 0.0f}, 0},
             {Vec3{x, 0.0f, -y}, Vec3{0.0f, 1.0f, 0.0f}, 0},
             {Vec3{x, 0.0f, y}, Vec3{0.0f, 1.0f, 0.0f}, 0}},
            {0, 1, 2, 2, 1, 3}};
}

Data cube(float r) {
    return {{{Vec3{-r, -r, -r}, Vec3{-r, -r, -r}.unit(), 0},
             {Vec3{r, -r, -r}, Vec3{r, -r, -r}.unit(), 0},
             {Vec3{r, r, -r}, Vec3{r, r, -r}.unit(), 0},
             {Vec3{-r, r, -r}, Vec3{-r, r, -r}.unit(), 0},
             {Vec3{-r, -r, r}, Vec3{-r, -r, r}.unit(), 0},
             {Vec3{r, -r, r}, Vec3{r, -r, r}.unit(), 0},
             {Vec3{r, r, r}, Vec3{r, r, r}.unit(), 0},
             {Vec3{-r, r, r}, Vec3{-r, r, r}.unit(), 0}},
            {0, 1, 3, 3, 1, 2, 1, 5, 2, 2, 5, 6, 5, 4, 6, 6, 4, 7,
             4, 0, 7, 7, 0, 3, 3, 2, 7, 7, 2, 6, 4, 5, 0, 0, 5, 1}};
}

// https://wiki.unity3d.com/index.php/ProceduralPrimitives
Data cone(float bradius, float tradius, float height, int sides, bool caps) {

    const size_t n_sides = sides, n_cap = n_sides + 1;
    const float _2pi = PI_F * 2.0f;

    std::vector<Vec3> vertices(n_cap + n_cap + n_sides * 2 + 2);
    size_t vert = 0;

    vertices[vert++] = Vec3(0.0f, 0.0f, 0.0f);
    while(vert <= n_sides) {
        float rad = (float)vert / n_sides * _2pi;
        vertices[vert] = Vec3(std::cos(rad) * bradius, 0.0f, std::sin(rad) * bradius);
        vert++;
    }
    vertices[vert++] = Vec3(0.0f, height, 0.0f);
    while(vert <= n_sides * 2 + 1) {
        float rad = (float)(vert - n_sides - 1) / n_sides * _2pi;
        vertices[vert] = Vec3(std::cos(rad) * tradius, height, std::sin(rad) * tradius);
        vert++;
    }
    size_t v = 0;
    while(vert <= vertices.size() - 4) {
        float rad = (float)v / n_sides * _2pi;
        vertices[vert] = Vec3(std::cos(rad) * tradius, height, std::sin(rad) * tradius);
        vertices[vert + 1] = Vec3(std::cos(rad) * bradius, 0.0f, std::sin(rad) * bradius);
        vert += 2;
        v++;
    }
    vertices[vert] = vertices[n_sides * 2 + 2];
    vertices[vert + 1] = vertices[n_sides * 2 + 3];

    std::vector<Vec3> normals(vertices.size());
    vert = 0;
    while(vert <= n_sides) {
        normals[vert++] = Vec3(0.0f, -1.0f, 0.0f);
    }
    while(vert <= n_sides * 2 + 1) {
        normals[vert++] = Vec3(0.0f, 1.0f, 0.0f);
    }

    v = 0;
    while(vert <= vertices.size() - 4) {
        float rad = (float)v / n_sides * _2pi;
        float cos = std::cos(rad);
        float sin = std::sin(rad);
        normals[vert] = Vec3(cos, 0.0f, sin);
        normals[vert + 1] = normals[vert];
        vert += 2;
        v++;
    }
    normals[vert] = normals[n_sides * 2 + 2];
    normals[vert + 1] = normals[n_sides * 2 + 3];

    size_t n_tris = n_sides + n_sides + n_sides * 2;
    std::vector<GL::Mesh::Index> triangles(n_tris * 3 + 3);

    GL::Mesh::Index tri = 0;
    size_t i = 0;
    while(tri < n_sides - 1) {
        if(caps) {
            triangles[i] = 0;
            triangles[i + 1] = tri + 1;
            triangles[i + 2] = tri + 2;
        }
        tri++;
        i += 3;
    }
    if(caps) {
        triangles[i] = 0;
        triangles[i + 1] = tri + 1;
        triangles[i + 2] = 1;
    }
    tri++;
    i += 3;

    while(tri < n_sides * 2) {
        if(caps) {
            triangles[i] = tri + 2;
            triangles[i + 1] = tri + 1;
            triangles[i + 2] = (GLuint)n_cap;
        }
        tri++;
        i += 3;
    }
    if(caps) {
        triangles[i] = (GLuint)n_cap + 1;
        triangles[i + 1] = tri + 1;
        triangles[i + 2] = (GLuint)n_cap;
    }
    tri++;
    i += 3;
    tri++;

    while(tri <= n_tris) {
        triangles[i] = tri + 2;
        triangles[i + 1] = tri + 1;
        triangles[i + 2] = tri + 0;
        tri++;
        i += 3;
        triangles[i] = tri + 1;
        triangles[i + 1] = tri + 2;
        triangles[i + 2] = tri + 0;
        tri++;
        i += 3;
    }

    std::vector<GL::Mesh::Vert> verts;
    for(size_t j = 0; j < vertices.size(); j++) {
        verts.push_back({vertices[j], normals[j], 0});
    }
    return {verts, triangles};
}

Data torus(float iradius, float oradius, int segments, int sides) {

    const int n_rad_sides = segments, n_sides = sides;
    const float _2pi = PI_F * 2.0f;
    iradius = oradius - iradius;

    std::vector<Vec3> vertices((n_rad_sides + 1) * (n_sides + 1));
    for(int seg = 0; seg <= n_rad_sides; seg++) {

        int cur_seg = seg == n_rad_sides ? 0 : seg;

        float t1 = (float)cur_seg / n_rad_sides * _2pi;
        Vec3 r1(std::cos(t1) * oradius, 0.0f, std::sin(t1) * oradius);

        for(int side = 0; side <= n_sides; side++) {

            int cur_side = side == n_sides ? 0 : side;
            float t2 = (float)cur_side / n_sides * _2pi;
            Vec3 r2 = Mat4::rotate(Degrees(-t1), Vec3{0.0f, 1.0f, 0.0f}) *
                      Vec3(std::sin(t2) * iradius, std::cos(t2) * iradius, 0.0f);

            vertices[side + seg * (n_sides + 1)] = r1 + r2;
        }
    }

    std::vector<Vec3> normals(vertices.size());
    for(int seg = 0; seg <= n_rad_sides; seg++) {

        int cur_seg = seg == n_rad_sides ? 0 : seg;
        float t1 = (float)cur_seg / n_rad_sides * _2pi;
        Vec3 r1(std::cos(t1) * oradius, 0.0f, std::sin(t1) * oradius);

        for(int side = 0; side <= n_sides; side++) {
            normals[side + seg * (n_sides + 1)] =
                (vertices[side + seg * (n_sides + 1)] - r1).unit();
        }
    }

    int n_faces = (int)vertices.size();
    int n_tris = n_faces * 2;
    int n_idx = n_tris * 3;
    std::vector<GL::Mesh::Index> triangles(n_idx);

    size_t i = 0;
    for(int seg = 0; seg <= n_rad_sides; seg++) {
        for(int side = 0; side <= n_sides - 1; side++) {

            int current = side + seg * (n_sides + 1);
            int next = side + (seg < (n_rad_sides) ? (seg + 1) * (n_sides + 1) : 0);

            if(i < triangles.size() - 6) {
                triangles[i++] = current;
                triangles[i++] = next;
                triangles[i++] = next + 1;
                triangles[i++] = current;
                triangles[i++] = next + 1;
                triangles[i++] = current + 1;
            }
        }
    }

    std::vector<GL::Mesh::Vert> verts;
    for(size_t j = 0; j < vertices.size(); j++) {
        verts.push_back({vertices[j], normals[j], 0});
    }
    return {verts, triangles};
}

Data uv_hemisphere(float radius) {
    int nbLong = 64;
    int nbLat = 16;

    std::vector<Vec3> vertices((nbLong + 1) * nbLat + 2);
    float _pi = PI_F;
    float _2pi = _pi * 2.0f;

    vertices[0] = Vec3{0.0f, radius, 0.0f};
    for(int lat = 0; lat < nbLat; lat++) {
        float a1 = _pi * (float)(lat + 1) / (nbLat + 1);
        float sin1 = std::sin(a1);
        float cos1 = std::cos(a1);

        for(int lon = 0; lon <= nbLong; lon++) {
            float a2 = _2pi * (float)(lon == nbLong ? 0 : lon) / nbLong;
            float sin2 = std::sin(a2);
            float cos2 = std::cos(a2);

            vertices[lon + lat * (nbLong + 1) + 1] = Vec3(sin1 * cos2, cos1, sin1 * sin2) * radius;
        }
    }
    vertices[vertices.size() - 1] = Vec3{0.0f, -radius, 0.0f};

    std::vector<Vec3> normals(vertices.size());
    for(size_t n = 0; n < vertices.size(); n++) normals[n] = vertices[n].unit();

    int nbFaces = (int)vertices.size();
    int nbTriangles = nbFaces * 2;
    int nbIndexes = nbTriangles * 3;
    std::vector<GL::Mesh::Index> triangles(nbIndexes);

    int i = 0;
    for(int lat = (nbLat - 1) / 2; lat < nbLat - 1; lat++) {
        for(int lon = 0; lon < nbLong; lon++) {
            int current = lon + lat * (nbLong + 1) + 1;
            int next = current + nbLong + 1;

            triangles[i++] = current;
            triangles[i++] = current + 1;
            triangles[i++] = next + 1;

            triangles[i++] = current;
            triangles[i++] = next + 1;
            triangles[i++] = next;
        }
    }

    for(int lon = 0; lon < nbLong; lon++) {
        triangles[i++] = (int)vertices.size() - 1;
        triangles[i++] = (int)vertices.size() - (lon + 2) - 1;
        triangles[i++] = (int)vertices.size() - (lon + 1) - 1;
    }

    std::vector<GL::Mesh::Vert> verts;
    for(size_t j = 0; j < vertices.size(); j++) {
        verts.push_back({vertices[j], normals[j], 0});
    }
    triangles.resize(i);
    return {verts, triangles};
}

Data ico_sphere(float radius, int level) {
    struct TriIdx {
        int v1, v2, v3;
    };

    auto middle_point = [&](int p1, int p2, std::vector<Vec3>& vertices,
                            std::map<int64_t, size_t>& cache, float radius) -> size_t {
        bool firstIsSmaller = p1 < p2;
        int64_t smallerIndex = firstIsSmaller ? p1 : p2;
        int64_t greaterIndex = firstIsSmaller ? p2 : p1;
        int64_t key = (smallerIndex << 32ll) + greaterIndex;

        auto entry = cache.find(key);
        if(entry != cache.end()) {
            return entry->second;
        }

        Vec3 point1 = vertices[p1];
        Vec3 point2 = vertices[p2];
        Vec3 middle((point1.x + point2.x) / 2.0f, (point1.y + point2.y) / 2.0f,
                    (point1.z + point2.z) / 2.0f);
        size_t i = vertices.size();
        vertices.push_back(middle.unit() * radius);
        cache[key] = i;
        return i;
    };

    std::vector<Vec3> vertices;
    std::map<int64_t, size_t> middlePointIndexCache;
    float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    vertices.push_back(Vec3(-1.0f, t, 0.0f).unit() * radius);
    vertices.push_back(Vec3(1.0f, t, 0.0f).unit() * radius);
    vertices.push_back(Vec3(-1.0f, -t, 0.0f).unit() * radius);
    vertices.push_back(Vec3(1.0f, -t, 0.0f).unit() * radius);
    vertices.push_back(Vec3(0.0f, -1.0f, t).unit() * radius);
    vertices.push_back(Vec3(0.0f, 1.0f, t).unit() * radius);
    vertices.push_back(Vec3(0.0f, -1.0f, -t).unit() * radius);
    vertices.push_back(Vec3(0.0f, 1.0f, -t).unit() * radius);
    vertices.push_back(Vec3(t, 0.0f, -1.0f).unit() * radius);
    vertices.push_back(Vec3(t, 0.0f, 1.0f).unit() * radius);
    vertices.push_back(Vec3(-t, 0.0f, -1.0f).unit() * radius);
    vertices.push_back(Vec3(-t, 0.0f, 1.0f).unit() * radius);

    std::vector<TriIdx> faces;
    faces.push_back(TriIdx{0, 11, 5});
    faces.push_back(TriIdx{0, 5, 1});
    faces.push_back(TriIdx{0, 1, 7});
    faces.push_back(TriIdx{0, 7, 10});
    faces.push_back(TriIdx{0, 10, 11});
    faces.push_back(TriIdx{1, 5, 9});
    faces.push_back(TriIdx{5, 11, 4});
    faces.push_back(TriIdx{11, 10, 2});
    faces.push_back(TriIdx{10, 7, 6});
    faces.push_back(TriIdx{7, 1, 8});
    faces.push_back(TriIdx{3, 9, 4});
    faces.push_back(TriIdx{3, 4, 2});
    faces.push_back(TriIdx{3, 2, 6});
    faces.push_back(TriIdx{3, 6, 8});
    faces.push_back(TriIdx{3, 8, 9});
    faces.push_back(TriIdx{4, 9, 5});
    faces.push_back(TriIdx{2, 4, 11});
    faces.push_back(TriIdx{6, 2, 10});
    faces.push_back(TriIdx{8, 6, 7});
    faces.push_back(TriIdx{9, 8, 1});

    for(int i = 0; i < level; i++) {
        std::vector<TriIdx> faces2;
        for(auto tri : faces) {
            int a = (int)middle_point(tri.v1, tri.v2, vertices, middlePointIndexCache, radius);
            int b = (int)middle_point(tri.v2, tri.v3, vertices, middlePointIndexCache, radius);
            int c = (int)middle_point(tri.v3, tri.v1, vertices, middlePointIndexCache, radius);
            faces2.push_back(TriIdx{tri.v1, a, c});
            faces2.push_back(TriIdx{tri.v2, b, a});
            faces2.push_back(TriIdx{tri.v3, c, b});
            faces2.push_back(TriIdx{a, b, c});
        }
        faces = faces2;
    }

    std::vector<GL::Mesh::Index> triangles;
    for(size_t i = 0; i < faces.size(); i++) {
        triangles.push_back(faces[i].v1);
        triangles.push_back(faces[i].v2);
        triangles.push_back(faces[i].v3);
    }

    std::vector<Vec3> normals(vertices.size());
    for(size_t i = 0; i < normals.size(); i++) normals[i] = vertices[i].unit();

    std::vector<GL::Mesh::Vert> verts;
    for(size_t i = 0; i < vertices.size(); i++) {
        verts.push_back({vertices[i], normals[i], 0});
    }
    return {verts, triangles};
}

} // namespace Gen
} // namespace Util
