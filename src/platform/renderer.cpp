
#include <imgui/imgui.h>

#include "../geometry/util.h"
#include "../gui/manager.h"
#include "../lib/mathlib.h"

#include "renderer.h"

static const uint32_t DEFAULT_SAMPLES = 4;

Renderer::Renderer(Vec2 dim)
	: framebuffer(2, dim, DEFAULT_SAMPLES, true), id_resolve(1, dim, 1, false),
	  save_buffer(1, dim, DEFAULT_SAMPLES, true), save_output(1, dim, 1, false),
	  outline_fb(0, dim, DEFAULT_SAMPLES, true),
	  mesh_shader(GL::Shaders::mesh_v, GL::Shaders::mesh_f),
	  line_shader(GL::Shaders::line_v, GL::Shaders::line_f),
	  inst_shader(GL::Shaders::inst_v, GL::Shaders::mesh_f),
	  dome_shader(GL::Shaders::dome_v, GL::Shaders::dome_f),
	  _sphere(Util::sphere_mesh(1.0f, 3).to_gl()),
	  _cyl(Util::cyl_mesh(1.0f, 1.0f, 64, false).to_gl()), _hemi(Util::hemi_mesh(1.0f).to_gl()),
	  samples(DEFAULT_SAMPLES), window_dim(dim),
	  id_buffer(new GLubyte[static_cast<int32_t>(dim.x) * static_cast<int32_t>(dim.y) * 4]) {
}

Renderer::~Renderer() {
	delete[] id_buffer;
	id_buffer = nullptr;
}

Renderer& Renderer::get() {
	assert(data);
	return *data;
}

void Renderer::setup(Vec2 dim) {
	data = new Renderer(dim);
}

void Renderer::update_dim(Vec2 dim) {

	window_dim = dim;
	delete[] id_buffer;
	id_buffer = new GLubyte[static_cast<int32_t>(dim.x) * static_cast<int32_t>(dim.y) * 4]();
	framebuffer.resize(dim, samples);
	outline_fb.resize(dim, samples);
	id_resolve.resize(dim);
}

void Renderer::shutdown() {
	delete data;
	data = nullptr;
}

void Renderer::proj(const Mat4& proj) {
	_proj = proj;
}

void Renderer::complete() {

	framebuffer.blit_to(1, id_resolve, false);

	if (!id_resolve.can_read_at()) id_resolve.read(0, id_buffer);

	framebuffer.blit_to_screen(0, window_dim);
}

void Renderer::begin() {

	framebuffer.clear(0, Gui::Color::background, 1.0f);
	framebuffer.clear(1, Gui::Color::black, 1.0f);
	framebuffer.clear_d();
	outline_fb.clear_d();
	framebuffer.bind();
	GL::viewport(window_dim);
}

void Renderer::save(Gui::Manager& manager, std::shared_ptr<Instance::Camera> inst) {

	if (!inst) return;
	if (inst->camera.expired()) return;
	auto cam = inst->camera.lock();

	Mat4 view = inst->transform.expired() ? Mat4::I : inst->transform.lock()->world_to_local();
	Mat4 old_proj = _proj;
	_proj = cam->projection();

	Vec2 dim(static_cast<float>(cam->film.width), static_cast<float>(cam->film.height));
	uint32_t s = std::min(cam->film.samples, GL::max_msaa());

	save_buffer.resize(dim, s);
	save_output.resize(dim);
	save_buffer.clear(0, Spectrum{0.0f}, 1.0f);
	save_buffer.clear_d();
	save_buffer.bind();
	GL::viewport(dim);

	manager.render_instances(view, false);

	save_buffer.blit_to(0, save_output, true);

	framebuffer.bind();

	_proj = old_proj;
	GL::viewport(window_dim);
}

void Renderer::saved(std::vector<uint8_t>& out) const {
	save_output.flush();
	out.resize(save_output.bytes());
	save_output.read(0, out.data());
}

GLuint Renderer::saved() const {
	save_output.flush();
	return save_output.get_output(0);
}

void Renderer::lines(const GL::Lines& lines, const Mat4& view, const Mat4& model, float alpha) {

	Mat4 mvp = _proj * view * model;
	line_shader.bind();
	line_shader.uniform("mvp", mvp);
	line_shader.uniform("alpha", alpha);
	lines.render(framebuffer.is_multisampled());
}

void Renderer::skydome(const Mat4& rotation, Spectrum color, float cosine, const GL::Tex2D& tex) {

	auto [tw, th] = tex.get_dim();
	Vec2 itex_size = 1.0f / Vec2(static_cast<float>(tw), static_cast<float>(th));

	tex.bind();
	dome_shader.bind();
	dome_shader.uniform("tex", 0);
	dome_shader.uniform("use_texture", true);
	dome_shader.uniform("itex_size", itex_size);
	dome_shader.uniform("color", color);
	dome_shader.uniform("cosine", cosine);
	dome_shader.uniform("transform", _proj * rotation);
	_sphere.render();
}

void Renderer::skydome(const Mat4& rotation, Spectrum color, float cosine) {

	dome_shader.bind();
	dome_shader.uniform("use_texture", false);
	dome_shader.uniform("color", color);
	dome_shader.uniform("cosine", cosine);
	dome_shader.uniform("transform", _proj * rotation);
	_sphere.render();
}

void Renderer::sphere(MeshOpt opt) {
	mesh(_sphere, opt);
}

void Renderer::capsule(MeshOpt opt, float height, float rad, BBox& box) {

	Mat4 cyl = opt.modelview * Mat4::scale(Vec3{rad, height, rad});
	Mat4 bot = opt.modelview * Mat4::scale(Vec3{rad});
	Mat4 top = opt.modelview * Mat4::translate(Vec3{0.0f, height, 0.0f}) *
	           Mat4::euler(Vec3{180.0f, 0.0f, 0.0f}) * Mat4::scale(Vec3{rad});

	opt.modelview = cyl;
	mesh(_cyl, opt);
	opt.modelview = bot;
	mesh(_hemi, opt);
	opt.modelview = top;
	mesh(_hemi, opt);

	BBox b = _cyl.bbox();
	b.transform(cyl);
	box.enclose(b);
	b = _hemi.bbox();
	b.transform(bot);
	box.enclose(b);
	b = _hemi.bbox();
	b.transform(top);
	box.enclose(b);
}

void Renderer::capsule(MeshOpt opt, float height, float rad) {
	BBox box;
	capsule(opt, height, rad, box);
}

void Renderer::mesh(GL::Mesh& mesh, Renderer::MeshOpt opt) {

	mesh_shader.bind();
	mesh_shader.uniform("use_v_id", opt.per_vert_id);
	mesh_shader.uniform("id", opt.id);
	mesh_shader.uniform("alpha", opt.alpha);
	mesh_shader.uniform("mvp", _proj * opt.modelview);
	mesh_shader.uniform("normal", Mat4::transpose(Mat4::inverse(opt.modelview)));
	mesh_shader.uniform("solid", opt.solid_color);
	mesh_shader.uniform("sel_color", opt.sel_color);
	mesh_shader.uniform("sel_id", opt.sel_id);
	mesh_shader.uniform("hov_color", opt.hov_color);
	mesh_shader.uniform("hov_id", opt.hov_id);
	mesh_shader.uniform("err_color", Vec3{1.0f});
	mesh_shader.uniform("err_id", 0u);
	mesh_shader.uniform("use_texture", opt.use_texture);
	mesh_shader.uniform("tex", 0);

	if (opt.depth_only) GL::color_mask(false);

	if (opt.wireframe) {
		mesh_shader.uniform("color", Vec3());
		GL::enable(GL::Opt::wireframe);
		mesh.render();
		GL::disable(GL::Opt::wireframe);
	}

	mesh_shader.uniform("color", opt.color);
	if (opt.wireframe && !opt.depth_only) glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //HACK
	mesh.render();
	if (opt.wireframe && !opt.depth_only) glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); //HACK

	if (opt.depth_only) GL::color_mask(true);
}

void Renderer::set_samples(uint32_t s) {
	samples = s;
	framebuffer.resize(window_dim, samples);
}

uint32_t Renderer::read_id(Vec2 pos) {

	if (pos.x < 0 || pos.x >= window_dim.x) return 0;
	if (pos.y < 0 || pos.y >= window_dim.y) return 0;

	uint32_t x = static_cast<uint32_t>(pos.x);
	uint32_t y = static_cast<uint32_t>(window_dim.y - pos.y - 1);

	if (id_resolve.can_read_at()) {

		GLubyte read[4] = {};
		id_resolve.read_at(0, x, y, read);
		return static_cast<uint32_t>(read[0]) | static_cast<uint32_t>(read[1]) << 8 |
		       static_cast<uint32_t>(read[2]) << 16;

	} else {

		uint32_t idx = y * static_cast<uint32_t>(window_dim.x) * 4 + x * 4;
		assert(id_buffer && idx > 0 && idx <= window_dim.x * window_dim.y * 4);

		uint32_t a = id_buffer[idx];
		uint32_t b = id_buffer[idx + 1];
		uint32_t c = id_buffer[idx + 2];

		return a | b << 8 | c << 16;
	}
}

void Renderer::reset_depth() {
	framebuffer.clear_d();
}

void Renderer::begin_outline() {
	outline_fb.clear_d();
	outline_fb.bind();
}

void Renderer::end_outline(BBox box) {

	Vec2 min, max;
	box.screen_rect(_proj, min, max);

	Vec2 thickness = Vec2(3.0f / window_dim.x, 3.0f / window_dim.y);
	GL::Effects::outline(outline_fb, framebuffer, Gui::Color::outline, min - thickness,
	                     max + thickness);

	framebuffer.bind();
}

void Renderer::instances(GL::Instances& inst, GL::Mesh& mesh, Renderer::MeshOpt opt) {

	inst_shader.bind();
	inst_shader.uniform("use_v_id", opt.per_vert_id);
	inst_shader.uniform("use_i_id", true);
	inst_shader.uniform("id", opt.id);
	inst_shader.uniform("alpha", opt.alpha);
	inst_shader.uniform("proj", _proj);
	inst_shader.uniform("modelview", opt.modelview);
	inst_shader.uniform("solid", opt.solid_color);
	inst_shader.uniform("sel_color", opt.sel_color);
	inst_shader.uniform("sel_id", opt.sel_id);
	inst_shader.uniform("hov_color", opt.hov_color);
	inst_shader.uniform("hov_id", opt.hov_id);
	inst_shader.uniform("err_color", Vec3{1.0f});
	inst_shader.uniform("err_id", 0u);
	inst_shader.uniform("use_texture", opt.use_texture);
	inst_shader.uniform("tex", 0);

	if (opt.depth_only) GL::color_mask(false);

	if (opt.wireframe) {
		inst_shader.uniform("color", Vec3());
		GL::enable(GL::Opt::wireframe);
		inst.render(mesh);
		GL::disable(GL::Opt::wireframe);
	}

	inst_shader.uniform("color", opt.color);
	inst.render(mesh);

	if (opt.depth_only) GL::color_mask(true);
}

void Renderer::halfedge_editor(Renderer::HalfedgeOpt opt) {

	auto [spheres, cylinders, arrows] = opt.editor.instances();
	auto [face_mesh, vert_mesh, edge_mesh, halfedge_mesh] = opt.editor.meshes();

	MeshOpt fopt = MeshOpt();
	fopt.modelview = opt.modelview;
	fopt.color = opt.f_color;
	fopt.per_vert_id = true;
	fopt.sel_color = Gui::Color::outline;
	fopt.sel_id = opt.sel_id;
	fopt.hov_color = Gui::Color::hover;
	fopt.hov_id = opt.hov_id;
	Renderer::mesh(face_mesh, fopt);

	inst_shader.bind();
	inst_shader.uniform("use_v_id", true);
	inst_shader.uniform("use_i_id", true);
	inst_shader.uniform("solid", false);
	inst_shader.uniform("proj", _proj);
	inst_shader.uniform("modelview", opt.modelview);
	inst_shader.uniform("alpha", fopt.alpha);
	inst_shader.uniform("sel_color", Gui::Color::outline);
	inst_shader.uniform("hov_color", Gui::Color::hover);
	inst_shader.uniform("sel_id", fopt.sel_id);
	inst_shader.uniform("hov_id", fopt.hov_id);

	inst_shader.uniform("err_color", opt.err_color);
	inst_shader.uniform("err_id", opt.err_id);

	inst_shader.uniform("color", opt.v_color);
	spheres.render(vert_mesh);
	inst_shader.uniform("color", opt.e_color);
	cylinders.render(edge_mesh);
	inst_shader.uniform("color", opt.he_color);
	arrows.render(halfedge_mesh);
}

Renderer::Skeleton_ID_Map Renderer::skeleton(Renderer::SkeletonOpt sopt) {

	uint32_t id = sopt.base_id;
	uint32_t root_id = id++;

	std::unordered_map<uint32_t, std::weak_ptr<Bone>> id_to_bone;
	std::unordered_map<uint32_t, std::weak_ptr<Skeleton::IK_Handle>> id_to_handle;
	std::unordered_map<std::shared_ptr<Bone>, uint32_t> bone_to_id;

	// Draw mesh
	if (sopt.face_mesh) {
		MeshOpt opt;
		opt.modelview = sopt.view;
		opt.color = Spectrum{1.0f};
		opt.id = id++;
		mesh(*sopt.face_mesh, opt);
	}

	// Draw bone outlines
	sopt.skeleton.for_bones([&](std::shared_ptr<Bone> b) {
		id_to_bone[id] = b;
		bone_to_id[b] = id;
		MeshOpt opt;
		opt.modelview = sopt.view *
		                (sopt.posed ? sopt.skeleton.j_to_posed(b) : sopt.skeleton.j_to_bind(b)) *
		                Mat4::rotate_to(b->extent);
		opt.id = id++;
		opt.alpha = 0.8f;
		opt.color = Gui::Color::hover;
		capsule(opt, b->extent.norm(), b->radius);
	});

	// Outline selected bone
	if (!sopt.selected_bone.expired()) {
		auto selected = sopt.selected_bone.lock();

		Mat4 model =
			(sopt.posed ? sopt.skeleton.j_to_posed(selected) : sopt.skeleton.j_to_bind(selected)) *
			Mat4::rotate_to(selected->extent);

		BBox box;
		MeshOpt opt;
		opt.modelview = sopt.view * model;
		opt.id = bone_to_id[selected];
		opt.depth_only = true;

		begin_outline();
		capsule(opt, selected->extent.norm(), selected->radius, box);
		end_outline(box);
	}
	reset_depth();

	// Render skeleton base point
	{
		MeshOpt opt;
		opt.id = root_id;
		opt.modelview = sopt.view * Mat4::translate(sopt.skeleton.base) * Mat4::scale(Vec3{0.1f});
		opt.color = sopt.root_selected ? Gui::Color::outline : Gui::Color::hover;
		sphere(opt);
	}

	// Draw bone base points
	sopt.skeleton.for_bones([&](std::shared_ptr<Bone> b) {
		MeshOpt opt;
		opt.modelview = sopt.view *
		                (sopt.posed ? sopt.skeleton.j_to_posed(b) : sopt.skeleton.j_to_bind(b)) *
		                Mat4::translate(b->extent) * Mat4::scale(Vec3{b->radius * 0.25f});
		opt.id = bone_to_id[b];
		opt.color = sopt.selected_bone.lock() == b ? Gui::Color::outline : Gui::Color::hover;
		sphere(opt);
	});

	// Draw IK handles
	{
		GL::Lines ik_lines;
		for (auto& h : sopt.skeleton.handles) {
			id_to_handle[id] = h;
			MeshOpt opt;
			opt.modelview = sopt.view * Mat4::translate(sopt.skeleton.base + h->target) *
			                Mat4::scale(Vec3{h->bone.lock()->radius * 0.3f});
			opt.id = id++;
			opt.color = sopt.selected_handle.lock() == h ? Gui::Color::outline : Gui::Color::hoverg;
			Vec3 j_world =
				sopt.posed ? sopt.skeleton.end_of_posed(h->bone) : sopt.skeleton.end_of(h->bone);
			ik_lines.add(h->target + sopt.skeleton.base, j_world,
			             h->enabled ? Spectrum(1.0f, 0.0f, 0.0f) : Spectrum(0.0f));
			sphere(opt);
		}
		lines(ik_lines, sopt.view);
	}

	return {id_to_bone, id_to_handle};
}
