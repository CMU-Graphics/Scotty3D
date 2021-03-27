
#include "gl.h"
#include "../lib/log.h"

#include <fstream>

namespace GL {

const char* Sample_Count_Names[(int)Sample_Count::count] = {"1", "2", "4", "8", "16", "32"};

int MSAA::n_options() {
    int max = max_msaa();
    if(max >= 32) return 6;
    if(max >= 16) return 5;
    if(max >= 8) return 4;
    if(max >= 4) return 3;
    if(max >= 2) return 2;
    return 1;
}

int MSAA::n_samples() {
    switch(samples) {
    case Sample_Count::_1: return 1;
    case Sample_Count::_2: return 2;
    case Sample_Count::_4: return 4;
    case Sample_Count::_8: return 8;
    case Sample_Count::_16: return 16;
    case Sample_Count::_32: return 32;
    default: assert(false);
    }
}

static void setup_debug_proc();
static void check_leaked_handles();
static bool is_gl45 = false;
static bool is_gl41 = false;

void setup() {
    std::string ver = version();
    is_gl45 = ver.find("4.5") != std::string::npos;
    is_gl41 = ver.find("4.1") != std::string::npos;

    setup_debug_proc();
    Effects::init();
}

void shutdown() {
    Effects::destroy();
    check_leaked_handles();
}

void color_mask(bool enable) {
    glColorMask(enable, enable, enable, enable);
}

std::string version() {
    return std::string((char*)glGetString(GL_VERSION));
}
std::string renderer() {
    return std::string((char*)glGetString(GL_RENDERER));
}

void global_params() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glClearDepth(0.0);
    if(glClipControl) glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glCullFace(GL_BACK);
}

void clear_screen(Vec4 col) {
    Framebuffer::bind_screen();
    glClearColor(col.x, col.y, col.z, col.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void enable(Opt opt) {
    switch(opt) {
    case Opt::wireframe: {
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } break;
    case Opt::offset: {
        glEnable(GL_POLYGON_OFFSET_FILL);
    } break;
    case Opt::culling: {
        glEnable(GL_CULL_FACE);
    } break;
    case Opt::depth_write: {
        glDepthMask(GL_TRUE);
    } break;
    }
}

void disable(Opt opt) {
    switch(opt) {
    case Opt::wireframe: {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
    } break;
    case Opt::offset: {
        glDisable(GL_POLYGON_OFFSET_FILL);
    } break;
    case Opt::culling: {
        glDisable(GL_CULL_FACE);
    } break;
    case Opt::depth_write: {
        glDepthMask(GL_FALSE);
    } break;
    }
}

void viewport(Vec2 dim) {
    glViewport(0, 0, (GLsizei)dim.x, (GLsizei)dim.y);
}

int max_msaa() {
    int samples;
    glGetIntegerv(GL_MAX_SAMPLES, &samples);
    return samples;
}

Tex2D::Tex2D() {
    id = 0;
}

Tex2D::Tex2D(Tex2D&& src) {
    id = src.id;
    src.id = 0;
}

Tex2D::~Tex2D() {
    if(id) glDeleteTextures(1, &id);
    id = 0;
}

void Tex2D::operator=(Tex2D&& src) {
    if(id) glDeleteTextures(1, &id);
    id = src.id;
    src.id = 0;
}

void Tex2D::bind(int idx) const {
    glActiveTexture(GL_TEXTURE0 + idx);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Tex2D::image(int w, int h, unsigned char* img) {
    if(!id) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

TexID Tex2D::get_id() const {
    return id;
}

Mesh::Mesh() {
    create();
}

Mesh::Mesh(std::vector<Vert>&& vertices, std::vector<Index>&& indices) {
    create();
    recreate(std::move(vertices), std::move(indices));
}

Mesh::Mesh(Mesh&& src) {
    vao = src.vao;
    src.vao = 0;
    ebo = src.ebo;
    src.ebo = 0;
    vbo = src.vbo;
    src.vbo = 0;
    dirty = src.dirty;
    src.dirty = true;
    n_elem = src.n_elem;
    src.n_elem = 0;
    _bbox = src._bbox;
    src._bbox.reset();
    _verts = std::move(src._verts);
    _idxs = std::move(src._idxs);
}

void Mesh::operator=(Mesh&& src) {
    destroy();
    vao = src.vao;
    src.vao = 0;
    vbo = src.vbo;
    src.vbo = 0;
    ebo = src.ebo;
    src.ebo = 0;
    dirty = src.dirty;
    src.dirty = true;
    n_elem = src.n_elem;
    src.n_elem = 0;
    _bbox = src._bbox;
    src._bbox.reset();
    _verts = std::move(src._verts);
    _idxs = std::move(src._idxs);
}

Mesh::~Mesh() {
    destroy();
}

void Mesh::create() {
    // Hack to let stuff get created for headless mode
    if(!glGenVertexArrays) return;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)sizeof(Vec3));
    glEnableVertexAttribArray(1);

    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(Vert), (GLvoid*)(2 * sizeof(Vec3)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBindVertexArray(0);
}

void Mesh::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if(!glDeleteBuffers) return;

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    ebo = vao = vbo = 0;
}

void Mesh::update() {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * _verts.size(), _verts.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Index) * _idxs.size(), _idxs.data(),
                 GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    dirty = false;
}

void Mesh::recreate(std::vector<Vert>&& vertices, std::vector<Index>&& indices) {

    dirty = true;
    _verts = std::move(vertices);
    _idxs = std::move(indices);

    _bbox.reset();
    for(auto& v : _verts) {
        _bbox.enclose(v.pos);
    }
    n_elem = (GLuint)_idxs.size();
}

GLuint Mesh::tris() const {
    return n_elem / 3;
}

Mesh Mesh::copy() const {
    std::vector<Vert> verts = _verts;
    std::vector<Index> idxs = _idxs;
    return Mesh(std::move(verts), std::move(idxs));
}

std::vector<Mesh::Vert>& Mesh::edit_verts() {
    dirty = true;
    return _verts;
}

std::vector<Mesh::Index>& Mesh::edit_indices() {
    dirty = true;
    return _idxs;
}

const std::vector<Mesh::Vert>& Mesh::verts() const {
    return _verts;
}

const std::vector<Mesh::Index>& Mesh::indices() const {
    return _idxs;
}

BBox Mesh::bbox() const {
    return _bbox;
}

void Mesh::render() {
    if(dirty) update();
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, n_elem, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Instances::Instances(Mesh&& mesh) : _mesh(std::move(mesh)) {
    create();
}

Instances::Instances(Instances&& src) {
    _mesh = std::move(src._mesh);
    data = std::move(src.data);
    vbo = src.vbo;
    src.vbo = 0;
    dirty = src.dirty;
    src.dirty = true;
}

Instances::~Instances() {
    destroy();
}

const Mesh& Instances::mesh() const {
    return _mesh;
}

void Instances::operator=(Instances&& src) {
    destroy();
    _mesh = std::move(src._mesh);
    data = std::move(src.data);
    vbo = src.vbo;
    src.vbo = 0;
    dirty = src.dirty;
    src.dirty = true;
}

void Instances::create() {
    // Hack to let stuff get created for headless mode
    if(!glGenBuffers) return;

    glGenBuffers(1, &vbo);
    glBindVertexArray(_mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(Info), (GLvoid*)0);
    glVertexAttribDivisor(3, 1);

    const int base_idx = 4;
    for(int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(base_idx + i);
        glVertexAttribPointer(base_idx + i, 4, GL_FLOAT, GL_FALSE, sizeof(Info),
                              (void*)(sizeof(GLuint) + sizeof(Vec4) * i));
        glVertexAttribDivisor(base_idx + i, 1);
    }
    glBindVertexArray(0);
}

void Instances::render() {

    if(_mesh.dirty) _mesh.update();
    if(dirty) update();

    glBindVertexArray(_mesh.vao);
    glDrawElementsInstanced(GL_TRIANGLES, _mesh.n_elem, GL_UNSIGNED_INT, nullptr,
                            (GLsizei)data.size());
    glBindVertexArray(0);
}

Instances::Info& Instances::get(size_t idx) {
    dirty = true;
    return data[idx];
}

size_t Instances::add(const Mat4& transform, GLuint id) {
    data.emplace_back(Info{id, transform});
    dirty = true;
    return data.size() - 1;
}

void Instances::clear(size_t n) {
    data.clear();
    if(n > 0) {
        data.reserve(n);
    }
    dirty = true;
}

void Instances::update() {
    glBindVertexArray(_mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Info) * data.size(), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    dirty = false;
}

void Instances::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if(!glDeleteBuffers) return;

    glDeleteBuffers(1, &vbo);
    vbo = 0;
    _mesh.destroy();
}

Lines::Lines(std::vector<Vert>&& verts, float thickness)
    : thickness(thickness), vertices(std::move(verts)) {
    create();
    dirty = true;
}

Lines::Lines(float thickness) : thickness(thickness) {
    create();
}

Lines::Lines(Lines&& src) {
    dirty = src.dirty;
    src.dirty = true;
    thickness = src.thickness;
    src.thickness = 0.0f;
    vao = src.vao;
    src.vao = 0;
    vbo = src.vbo;
    src.vbo = 0;
    vertices = std::move(src.vertices);
}

void Lines::operator=(Lines&& src) {
    destroy();
    dirty = src.dirty;
    src.dirty = true;
    thickness = src.thickness;
    src.thickness = 0.0f;
    vao = src.vao;
    src.vao = 0;
    vbo = src.vbo;
    src.vbo = 0;
    vertices = std::move(src.vertices);
}

Lines::~Lines() {
    destroy();
}

void Lines::update() const {

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    dirty = false;
}

void Lines::render(bool smooth) const {

    if(dirty) update();

    glLineWidth(thickness);
    if(smooth)
        glEnable(GL_LINE_SMOOTH);
    else
        glDisable(GL_LINE_SMOOTH);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertices.size());
    glBindVertexArray(0);
}

void Lines::clear() {
    vertices.clear();
    dirty = true;
}

void Lines::pop() {
    vertices.pop_back();
    vertices.pop_back();
    dirty = true;
}

void Lines::add(Vec3 start, Vec3 end, Vec3 color) {

    vertices.push_back({start, color});
    vertices.push_back({end, color});
    dirty = true;
}

void Lines::create() {
    // Hack to let stuff get created for headless mode
    if(!glGenBuffers) return;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)sizeof(Vec3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Lines::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if(!glDeleteBuffers) return;

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    vao = vbo = 0;
    vertices.clear();
    dirty = false;
}

Shader::Shader() {
}

Shader::Shader(std::string vertex, std::string fragment) {
    load(vertex, fragment);
}

Shader::Shader(Shader&& src) {
    program = src.program;
    src.program = 0;
    v = src.v;
    src.v = 0;
    f = src.f;
    src.f = 0;
}

void Shader::operator=(Shader&& src) {
    destroy();
    program = src.program;
    src.program = 0;
    v = src.v;
    src.v = 0;
    f = src.f;
    src.f = 0;
}

Shader::~Shader() {
    destroy();
}

void Shader::bind() const {
    glUseProgram(program);
}

void Shader::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if(!glUseProgram) return;

    glUseProgram(0);
    glDeleteShader(v);
    glDeleteShader(f);
    glDeleteProgram(program);
    v = f = program = 0;
}

void Shader::uniform_block(std::string name, GLuint i) const {
    GLuint idx = glGetUniformBlockIndex(program, name.c_str());
    glUniformBlockBinding(program, idx, i);
}

void Shader::uniform(std::string name, int count, const Vec2 items[]) const {
    glUniform2fv(loc(name), count, (GLfloat*)items);
}

void Shader::uniform(std::string name, GLfloat fl) const {
    glUniform1f(loc(name), fl);
}

void Shader::uniform(std::string name, const Mat4& mat) const {
    glUniformMatrix4fv(loc(name), 1, GL_FALSE, mat.data);
}

void Shader::uniform(std::string name, Vec3 vec3) const {
    glUniform3fv(loc(name), 1, vec3.data);
}

void Shader::uniform(std::string name, Vec2 vec2) const {
    glUniform2fv(loc(name), 1, vec2.data);
}

void Shader::uniform(std::string name, GLint i) const {
    glUniform1i(loc(name), i);
}

void Shader::uniform(std::string name, GLuint i) const {
    glUniform1ui(loc(name), i);
}

void Shader::uniform(std::string name, bool b) const {
    glUniform1i(loc(name), b);
}

GLuint Shader::loc(std::string name) const {
    return glGetUniformLocation(program, name.c_str());
}

void Shader::load(std::string vertex, std::string fragment) {

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vs_c = vertex.c_str();
    const GLchar* fs_c = fragment.c_str();
    glShaderSource(v, 1, &vs_c, NULL);
    glShaderSource(f, 1, &fs_c, NULL);
    glCompileShader(v);
    glCompileShader(f);

    if(!validate(v)) {
        destroy();
        return;
    }
    if(!validate(f)) {
        destroy();
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);
}

bool Shader::validate(GLuint program) {

    GLint compiled = 0;
    glGetShaderiv(program, GL_COMPILE_STATUS, &compiled);
    if(compiled == GL_FALSE) {

        GLint len = 0;
        glGetShaderiv(program, GL_INFO_LOG_LENGTH, &len);

        GLchar* msg = new GLchar[len];
        glGetShaderInfoLog(program, len, &len, msg);

        warn("Shader %d failed to compile: %s", program, msg);
        delete[] msg;

        return false;
    }
    return true;
}

Framebuffer::Framebuffer() {
}

Framebuffer::Framebuffer(int outputs, Vec2 dim, int samples, bool d) {
    setup(outputs, dim, samples, d);
}

void Framebuffer::setup(int outputs, Vec2 dim, int samples, bool d) {
    destroy();
    assert(outputs >= 0 && outputs < 31);
    depth = d;
    output_textures.resize(outputs);
    resize(dim, samples);
}

Framebuffer::Framebuffer(Framebuffer&& src) {
    output_textures = std::move(src.output_textures);
    depth_tex = src.depth_tex;
    src.depth_tex = 0;
    framebuffer = src.framebuffer;
    src.framebuffer = 0;
    w = src.w;
    src.w = 0;
    h = src.h;
    src.h = 0;
    s = src.s;
    src.s = 0;
}

void Framebuffer::operator=(Framebuffer&& src) {
    destroy();
    output_textures = std::move(src.output_textures);
    depth_tex = src.depth_tex;
    src.depth_tex = 0;
    framebuffer = src.framebuffer;
    src.framebuffer = 0;
    w = src.w;
    src.w = 0;
    h = src.h;
    src.h = 0;
    s = src.s;
    src.s = 0;
}

Framebuffer::~Framebuffer() {
    destroy();
}

void Framebuffer::create() {
    // Hack to let stuff get created for headless mode
    if(!glGenFramebuffers) return;

    glGenFramebuffers(1, &framebuffer);
    glGenTextures((GLsizei)output_textures.size(), (GLuint*)output_textures.data());
    if(depth) glGenTextures(1, &depth_tex);
}

void Framebuffer::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if(!glDeleteFramebuffers) return;

    glDeleteTextures(1, &depth_tex);
    glDeleteTextures((GLsizei)output_textures.size(), (GLuint*)output_textures.data());
    glDeleteFramebuffers(1, &framebuffer);
    depth_tex = framebuffer = 0;
}

void Framebuffer::resize(Vec2 dim, int samples) {

    destroy();
    create();

    w = (int)dim.x;
    h = (int)dim.y;
    s = samples;
    assert(w > 0 && h > 0 && s > 0);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLenum type = samples == 1 ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE;

    std::vector<GLenum> draw_buffers;

    for(GLenum i = 0; i < output_textures.size(); i++) {

        glBindTexture(type, output_textures[i]);

        if(s > 1) {
            glTexImage2DMultisample(type, s, GL_RGB8, w, h, GL_TRUE);
        } else {
            glTexImage2D(type, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, type, output_textures[i],
                               0);

        draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + i);

        glBindTexture(type, 0);
    }

    if(depth) {
        glBindTexture(type, depth_tex);
        if(s > 1) {
            glTexImage2DMultisample(type, s, GL_DEPTH_COMPONENT32F, w, h, GL_TRUE);
        } else {
            glTexImage2D(type, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, type, depth_tex, 0);
        glBindTexture(type, 0);
    }

    glDrawBuffers((GLsizei)draw_buffers.size(), draw_buffers.data());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::clear(int buf, Vec4 col) const {
    assert(buf >= 0 && buf < (int)output_textures.size());
    bind();
    glClearBufferfv(GL_COLOR, buf, col.data);
}

void Framebuffer::clear_d() const {
    bind();
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::bind_screen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

GLuint Framebuffer::get_output(int buf) const {
    assert(buf >= 0 && buf < (int)output_textures.size());
    return output_textures[buf];
}

int Framebuffer::bytes() const {
    return w * h * 4;
}

int Framebuffer::samples() const {
    return s;
}

void Framebuffer::flush() const {
    glFlush();
}

GLuint Framebuffer::get_depth() const {
    assert(depth_tex);
    return depth_tex;
}

bool Framebuffer::can_read_at() const {
    return is_gl45 && s == 1;
}

void Framebuffer::read_at(int buf, int x, int y, GLubyte* data) const {
    assert(can_read_at());
    assert(buf >= 0 && buf < (int)output_textures.size());
    glGetTextureSubImage(output_textures[buf], 0, x, y, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 4,
                         data);
}

void Framebuffer::read(int buf, GLubyte* data) const {
    assert(s == 1);
    assert(buf >= 0 && buf < (int)output_textures.size());
    glBindTexture(GL_TEXTURE_2D, output_textures[buf]);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void Framebuffer::blit_to(int buf, const Framebuffer& fb, bool avg) const {

    assert(buf >= 0 && buf < (int)output_textures.size());
    if(s > 1) {
        Effects::resolve_to(buf, *this, fb, avg);
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.framebuffer);

    glReadBuffer(GL_COLOR_ATTACHMENT0 + buf);
    glBlitFramebuffer(0, 0, w, h, 0, 0, fb.w, fb.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::blit_to_screen(int buf, Vec2 dim) const {

    assert(buf >= 0 && buf < (int)output_textures.size());
    if(s > 1) {
        Effects::resolve_to_screen(buf, *this);
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glReadBuffer(GL_COLOR_ATTACHMENT0 + buf);
    glBlitFramebuffer(0, 0, w, h, 0, 0, (GLint)dim.x, (GLint)dim.y, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Framebuffer::is_multisampled() const {
    return s > 1;
}

void Effects::init() {
    // Hack to let stuff get created for headless mode
    if(!glGenVertexArrays) return;

    glGenVertexArrays(1, &vao);
    resolve_shader.load(effects_v, resolve_f);
    outline_shader.load(effects_v, outline_f);
    outline_shader_ms.load(effects_v, is_gl45 || is_gl41 ? outline_ms_f_4 : outline_ms_f_33);
}

void Effects::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if(!glDeleteVertexArrays) return;

    glDeleteVertexArrays(1, &vao);
    vao = 0;
    resolve_shader.~Shader();
    outline_shader.~Shader();
    outline_shader_ms.~Shader();
}

void Effects::outline(const Framebuffer& from, const Framebuffer& to, Vec3 color, Vec2 min,
                      Vec2 max) {

    glFlush();
    to.bind();

    Vec2 quad[] = {Vec2{min.x, max.y}, min, max, Vec2{max.x, min.y}};

    if(from.is_multisampled()) {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, from.get_depth());
        outline_shader_ms.bind();
        outline_shader_ms.uniform("depth", 0);
        outline_shader_ms.uniform("color", color);
        outline_shader_ms.uniform("bounds", 4, quad);
    } else {
        glBindTexture(GL_TEXTURE_2D, from.get_depth());
        outline_shader.bind();
        outline_shader.uniform("depth", 0);
        outline_shader.uniform("color", color);
        outline_shader.uniform("i_screen_size", 1.0f / Vec2(from.w, from.h));
        outline_shader.uniform("bounds", 4, quad);
    }

    glBindVertexArray(vao);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glFlush();
}

void Effects::resolve_to_screen(int buf, const Framebuffer& framebuffer) {

    Framebuffer::bind_screen();

    resolve_shader.bind();

    assert(buf >= 0 && buf < (int)framebuffer.output_textures.size());
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebuffer.output_textures[buf]);

    resolve_shader.uniform("tex", 0);
    resolve_shader.uniform("samples", framebuffer.s);
    resolve_shader.uniform("bounds", 4, screen_quad);

    glBindVertexArray(vao);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void Effects::resolve_to(int buf, const Framebuffer& from, const Framebuffer& to, bool avg) {

    to.bind();

    resolve_shader.bind();

    assert(buf >= 0 && buf < (int)from.output_textures.size());
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, from.output_textures[buf]);

    resolve_shader.uniform("tex", 0);
    resolve_shader.uniform("samples", avg ? from.s : 1);
    resolve_shader.uniform("bounds", 4, screen_quad);

    glBindVertexArray(vao);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

static void debug_proc(GLenum glsource, GLenum gltype, GLuint, GLenum severity, GLsizei,
                       const GLchar* glmessage, const void*) {

    std::string message(glmessage);
    std::string source, type;

    switch(glsource) {
    case GL_DEBUG_SOURCE_API: source = "OpenGL API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: source = "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: source = "Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION: source = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER: source = "Other"; break;
    }

    switch(gltype) {
    case GL_DEBUG_TYPE_ERROR: type = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type = "Deprecated"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type = "Undefined Behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY: type = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: type = "Performance"; break;
    case GL_DEBUG_TYPE_MARKER: type = "Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP: type = "Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP: type = "Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER: type = "Other"; break;
    }

    switch(severity) {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM:
        warn("OpenGL | source: %s type: %s message: %s", source.c_str(), type.c_str(),
             message.c_str());
        break;
    }
}

static void check_leaked_handles() {

#define GL_CHECK(type)                                                                             \
    if(glIs##type && glIs##type(i) == GL_TRUE) {                                                   \
        warn("Leaked OpenGL handle %u of type %s", i, #type);                                      \
        leaked = true;                                                                             \
    }

    bool leaked = false;
    for(GLuint i = 0; i < 10000; i++) {
        GL_CHECK(Texture);
        GL_CHECK(Buffer);
        GL_CHECK(Framebuffer);
        GL_CHECK(Renderbuffer);
        GL_CHECK(VertexArray);
        GL_CHECK(Program);
        GL_CHECK(ProgramPipeline);
        GL_CHECK(Query);

        if(glIsShader(i) == GL_TRUE) {

            leaked = true;
            GLint shader_len = 0;
            glGetShaderiv(i, GL_SHADER_SOURCE_LENGTH, &shader_len);

            GLchar* shader = new GLchar[shader_len];
            glGetShaderSource(i, shader_len, nullptr, shader);

            warn("Leaked OpenGL shader %u. Source: %s", i, shader);

            delete[] shader;
        }
    }

    if(leaked) {
        warn("Leaked OpenGL objects!");
    }

#undef GL_CHECK
}

static void setup_debug_proc() {
    if(!glDebugMessageCallback || !glDebugMessageControl) return;
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debug_proc, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

const std::string Effects::effects_v = R"(
#version 330 core

layout (location = 0) in vec2 v_pos;

uniform vec2 bounds[4];

void main() {
	gl_Position = vec4(bounds[gl_VertexID], 0.0f, 1.0f);
})";
const std::string Effects::outline_f = R"(
#version 330 core

uniform sampler2D depth;
uniform vec3 color;
uniform vec2 i_screen_size;
out vec4 out_color;

void main() {

	ivec2 coord = ivec2(gl_FragCoord.xy);
	float o = texture(depth, coord * i_screen_size).r;

	float low = 1.0f / 0.0f;
	for (int i = -2; i <= 2; i++) {
		for (int j = -2; j <= 2; j++) {
			float d = texture(depth, (coord + ivec2(i,j)) * i_screen_size).r;
			low = min(low, d);
		}
	}

	float a = o != 0.0f && low == 0.0f ? 1.0f : 0.0f;
	out_color = vec4(color * a, a);
})";
const std::string Effects::outline_ms_f_4 = R"(
#version 400 core

uniform sampler2DMS depth;
uniform vec3 color;
out vec4 out_color;

void main() {

	ivec2 coord = ivec2(gl_FragCoord.xy);
	float o = texelFetch(depth, coord, gl_SampleID).r;

	float low = 1.0f / 0.0f;
	for (int i = -2; i <= 2; i++) {
		for (int j = -2; j <= 2; j++) {
			float d = texelFetch(depth, coord + ivec2(i,j), gl_SampleID).r;
			low = min(low, d);
		}
	}

	float a = o != 0.0f && low == 0.0f ? 1.0f : 0.0f;
	out_color = vec4(color * a, a);
})";
const std::string Effects::outline_ms_f_33 = R"(
#version 330 core

uniform sampler2DMS depth;
uniform vec3 color;
out vec4 out_color;

void main() {

	ivec2 coord = ivec2(gl_FragCoord.xy);
	float o = texelFetch(depth, coord, 0).r;

	float low = 1.0f / 0.0f;
	for (int i = -2; i <= 2; i++) {
		for (int j = -2; j <= 2; j++) {
			float d = texelFetch(depth, coord + ivec2(i,j), 0).r;
			low = min(low, d);
		}
	}

	float a = o != 0.0f && low == 0.0f ? 1.0f : 0.0f;
	out_color = vec4(color * a, a);
})";
const std::string Effects::resolve_f = R"(
#version 330 core

uniform sampler2DMS tex;
uniform int samples;
out vec4 out_color;

void main() {

	ivec2 coord = ivec2(gl_FragCoord.xy);

	vec3 color = vec3(0.0);

	for (int i = 0; i < samples; i++)
		color += texelFetch(tex, coord, i).xyz;

	color /= float(samples);

	out_color = vec4(color, 1.0f);
})";

namespace Shaders {
const std::string line_v = R"(
#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_col;

uniform mat4 mvp;
smooth out vec3 f_col;

void main() {
	gl_Position = mvp * vec4(v_pos, 1.0f);
	f_col = v_col;
})";
const std::string line_f = R"(
#version 330 core

layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_id;

uniform float alpha;
smooth in vec3 f_col;

void main() {
	out_id = vec4(0.0f);
	out_col = vec4(f_col, alpha);
})";
const std::string mesh_v = R"(
#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
layout (location = 2) in uint v_id;

uniform mat4 mvp, normal;

smooth out vec3 f_norm;
flat out uint f_id;

void main() {
	f_id = v_id;
	f_norm = (normal * vec4(v_norm, 0.0f)).xyz;
	gl_Position = mvp * vec4(v_pos, 1.0f);
})";
const std::string inst_v = R"(
#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
layout (location = 2) in uint v_id;

layout (location = 3) in uint i_id;
layout (location = 4) in mat4 i_trans;

uniform bool use_i_id;
uniform mat4 proj, modelview;

smooth out vec3 f_norm;
flat out uint f_id;

void main() {
	f_id = use_i_id ? i_id : v_id;
	mat4 mv = modelview * i_trans;
	mat4 n = transpose(inverse(mv));
	f_norm = (n * vec4(v_norm, 0.0f)).xyz;
	gl_Position = proj * mv * vec4(v_pos, 1.0f);
})";
const std::string mesh_f = R"(
#version 330 core

uniform bool solid, use_v_id;
uniform float alpha;
uniform uint id, sel_id, hov_id, err_id;
uniform vec3 color, sel_color, hov_color, err_color;

layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_id;

smooth in vec3 f_norm;
flat in uint f_id;

void main() {

	vec3 use_color;
	if(use_v_id) {
		out_id = vec4((f_id & 0xffu) / 255.0f, ((f_id >> 8) & 0xffu) / 255.0f, ((f_id >> 16) & 0xffu) / 255.0f, 1.0f);
        if(f_id == sel_id) {
            use_color = sel_color;
        } else if(f_id == hov_id) {
            use_color = hov_color;
        } else if (f_id == err_id) {
            use_color = err_color;
        } else {
		    use_color = color;
        }
	} else {
		out_id = vec4((id & 0xffu) / 255.0f, ((id >> 8) & 0xffu) / 255.0f, ((id >> 16) & 0xffu) / 255.0f, 1.0f);
        if(id == sel_id) {
            use_color = sel_color;
        } else if(id == hov_id) {
            use_color = hov_color;
		} else if (id == err_id) {
            use_color = err_color;
        } else {
		    use_color = color;
        }
	}

	if(solid) {
		out_col = vec4(use_color, alpha);
	} else {
		float ndotl = abs(normalize(f_norm).z);
		float light = clamp(0.3f + 0.6f * ndotl, 0.0f, alpha);
		out_col = vec4(light * use_color, alpha);
	}
})";
const std::string dome_v = R"(
#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
layout (location = 2) in uint v_id;

uniform mat4 transform;
smooth out vec3 f_pos;

void main() {
	f_pos = v_pos;
	vec4 pos = transform * vec4(v_pos, 1.0f);
	gl_Position = vec4(pos.xy, 0.000000001f, pos.w);
})";
const std::string dome_f = R"(
#version 330 core

#define PI 3.1415926535f
#define TAU 6.28318530718f

uniform vec3 color;
uniform float cosine;
uniform bool use_texture;
uniform sampler2D tex;

layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_id;

smooth in vec3 f_pos;

void main() {
	vec3 pos = normalize(f_pos);
	if(pos.y > cosine) {
		if(use_texture) {
			float theta = atan(pos.z,pos.x) / TAU + 0.5f;
			float phi = acos(pos.y) / PI;
			out_col = texture(tex, vec2(theta,phi));
		} else {
			out_col = vec4(color, 1.0f);
		}
	}
	else discard;
})";

} // namespace Shaders
} // namespace GL
