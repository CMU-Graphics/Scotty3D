
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../lib/mathlib.h"
#include <glad/glad.h>

namespace GL {

enum class Sample_Count : uint8_t { _1, _2, _4, _8, _16, _32, count };
extern const char* Sample_Count_Names[static_cast<uint8_t>(Sample_Count::count)];

struct MSAA {
	Sample_Count samples = Sample_Count::_4;
	uint32_t n_options();
	uint32_t n_samples();
};

void setup();
void shutdown();
std::string version();
std::string renderer();

void global_params();
void clear_screen(Vec4 col);
void viewport(Vec2 dim);
uint32_t max_msaa();

enum class Opt { wireframe, offset, culling, depth_write };

void enable(Opt opt);
void disable(Opt opt);

void color_mask(bool enable);

void depth_range(float n, float r); //depth values [-1,1] map to [near,far] in window coords; default is [0,1]

using TexID = GLuint;

class Tex2D {
public:
	Tex2D();
	Tex2D(const Tex2D& src) = delete;
	Tex2D(Tex2D&& src);
	~Tex2D();

	void operator=(const Tex2D& src) = delete;
	void operator=(Tex2D&& src);

	void bind(uint32_t idx = 0) const;
	void image(uint32_t w, uint32_t h, uint8_t* img);

	TexID get_id() const;
	std::pair<uint32_t, uint32_t> get_dim() const;

private:
	uint32_t w = 0, h = 0;
	GLuint id;
};

class Mesh {
public:
	typedef GLuint Index;
	struct Vert {
		Vec3 pos;
		Vec3 norm;
		Vec2 uv;
		GLuint id;
	};

	Mesh();
	Mesh(std::vector<Vert>&& vertices, std::vector<Index>&& indices);
	Mesh(const Mesh& src) = delete;
	Mesh(Mesh&& src);
	~Mesh();

	void operator=(const Mesh& src) = delete;
	void operator=(Mesh&& src);

	/// Assumes proper shader is already bound
	void render();

	void recreate(std::vector<Vert>&& vertices, std::vector<Index>&& indices);
	std::vector<Vert>& edit_verts();
	std::vector<Index>& edit_indices();
	Mesh copy() const;

	BBox bbox() const;
	const std::vector<Vert>& verts() const;
	const std::vector<Index>& indices() const;
	GLuint tris() const;

private:
	void update();
	void create();
	void destroy();

	BBox _bbox;
	GLuint vao = 0, vbo = 0, ebo = 0;
	GLuint n_elem = 0;
	bool dirty = true;

	std::vector<Vert> _verts;
	std::vector<Index> _idxs;

	friend class Instances;
};

class Instances {
public:
	Instances();
	Instances(const Instances& src) = delete;
	Instances(Instances&& src);
	~Instances();

	void operator=(const Instances& src) = delete;
	void operator=(Instances&& src);

	struct Info {
		GLuint id;
		Mat4 transform;
	};

	void render(GL::Mesh& mesh, std::vector< uint32_t > *include = nullptr, std::vector< uint32_t > *exclude = nullptr);
	size_t add(const Mat4& transform, GLuint id = 0);
	Info& get(size_t idx);
	void clear(size_t n = 0);

private:
	void create();
	void destroy();
	void update();

	GLuint vao = 0, vbo = 0;
	bool dirty = false;

	std::vector<Info> data;
};

class Lines {
public:
	struct Vert {
		Vec3 pos;
		Spectrum color;
	};

	Lines(float thickness = 1.0f);
	Lines(std::vector<Vert>&& verts, float thickness = 1.0f);
	Lines(const Lines& src) = delete;
	Lines(Lines&& src);
	~Lines();

	void operator=(const Lines& src) = delete;
	void operator=(Lines&& src);

	/// Assumes proper shader is already bound
	void render(bool smooth) const;
	void add(Vec3 start, Vec3 end, Spectrum color);
	void pop();
	void clear();

private:
	void create();
	void destroy();
	void update() const;

	mutable bool dirty = false;
	float thickness = 0.0f;
	GLuint vao = 0, vbo = 0;

	std::vector<Vert> vertices;
};

class Shader {
public:
	Shader();
	Shader(std::string vertex_file, std::string fragment_file);
	Shader(const Shader& src) = delete;
	Shader(Shader&& src);
	~Shader();

	void operator=(const Shader& src) = delete;
	void operator=(Shader&& src);

	void bind() const;
	void load(std::string vertex, std::string fragment);

	void uniform(std::string name, const Mat4& mat) const;
	void uniform(std::string name, Vec3 vec3) const;
	void uniform(std::string name, Spectrum color) const;
	void uniform(std::string name, Vec2 vec2) const;
	void uniform(std::string name, GLint i) const;
	void uniform(std::string name, GLuint i) const;
	void uniform(std::string name, GLfloat f) const;
	void uniform(std::string name, bool b) const;
	void uniform(std::string name, uint32_t count, const Vec2 items[]) const;
	void uniform_block(std::string name, GLuint i) const;

private:
	GLuint loc(std::string name) const;
	static bool validate(GLuint program);

	GLuint program = 0;
	GLuint v = 0, f = 0;

	void destroy();
};

/// this is very restrictive; it assumes a set number of gl_rgb8 output
/// textures and a floating point depth render buffer.
class Framebuffer {
public:
	Framebuffer();
	Framebuffer(uint32_t outputs, Vec2 dim, uint32_t samples = 1, bool depth = true);
	Framebuffer(const Framebuffer& src) = delete;
	Framebuffer(Framebuffer&& src);
	~Framebuffer();

	void operator=(const Framebuffer& src) = delete;
	void operator=(Framebuffer&& src);

	static void bind_screen();

	void setup(uint32_t outputs, Vec2 dim, uint32_t samples, bool d);
	void resize(Vec2 dim, uint32_t samples = 1);
	void bind() const;
	bool is_multisampled() const;

	GLuint get_output(uint32_t buf) const;
	GLuint get_depth() const;
	void flush() const;
	uint32_t samples() const;
	uint32_t bytes() const;

	bool can_read_at() const;
	void read_at(uint32_t buf, uint32_t x, uint32_t y, GLubyte* data) const;
	void read(uint32_t buf, GLubyte* data) const;

	void blit_to_screen(uint32_t buf, Vec2 dim) const;
	void blit_to(uint32_t buf, const Framebuffer& fb, bool avg = true) const;

	void clear(uint32_t buf, Spectrum col, float a) const;
	void clear_d() const;

private:
	void create();
	void destroy();

	std::vector<GLuint> output_textures;
	GLuint depth_tex = 0;
	GLuint framebuffer = 0;

	uint32_t w = 0, h = 0, s = 0;
	bool depth = true;

	friend class Effects;
};

class Effects {
public:
	static void resolve_to_screen(uint32_t buf, const Framebuffer& framebuffer);
	static void resolve_to(uint32_t buf, const Framebuffer& from, const Framebuffer& to,
	                       bool avg = true);

	static void outline(const Framebuffer& from, const Framebuffer& to, Spectrum color, Vec2 min,
	                    Vec2 max);

private:
	static void init();
	static void destroy();

	static inline Shader resolve_shader, outline_shader, outline_shader_ms;
	static inline GLuint vao;
	static inline const Vec2 screen_quad[] = {Vec2{-1.0f, 1.0f}, Vec2{-1.0f, -1.0f},
	                                          Vec2{1.0f, 1.0f}, Vec2{1.0f, -1.0f}};

	friend void setup();
	friend void shutdown();

	static const std::string effects_v;
	static const std::string outline_f, outline_ms_f_33, outline_ms_f_4;
	static const std::string resolve_f;
};

namespace Shaders {
extern const std::string line_v, line_f;
extern const std::string mesh_v, mesh_f;
extern const std::string inst_v;
extern const std::string dome_v, dome_f;

} // namespace Shaders
} // namespace GL
