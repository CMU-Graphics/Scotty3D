
#pragma once

#include <variant>

#include "../lib/mathlib.h"
#include "../platform/gl.h"
#include "../scene/instance.h"

namespace Gui {
class Manager;
class Model;
} // namespace Gui

// Singleton
class Renderer {
public:
	static void setup(Vec2 dim);
	static void shutdown();
	static Renderer& get();

	void begin();
	void complete();
	void reset_depth();

	void proj(const Mat4& proj);
	void update_dim(Vec2 dim);
	void settings_gui(bool* open);
	void set_samples(uint32_t samples);
	uint32_t read_id(Vec2 pos);

	struct MeshOpt {
		std::vector< uint32_t > sel_ids;
		uint32_t id = 0, active_id = 0, hov_id = 0;
		Mat4 modelview;
		Spectrum color, sel_color, hov_color;
		float alpha = 1.0f;
		bool wireframe = false;
		bool use_texture = false;
		bool solid_color = false;
		bool depth_only = false;
		bool per_vert_id = false;
	};

	struct HalfedgeOpt {
		HalfedgeOpt(Gui::Model& e) : editor(e) {
		}
		Gui::Model& editor;
		Mat4 modelview;
		Spectrum f_color = Spectrum{1.0f};
		Spectrum v_color = Spectrum{1.0f};
		Spectrum e_color = Spectrum{0.8f};
		Spectrum he_color = Spectrum{0.6f};
		Spectrum err_color = Spectrum{1.0f, 0.0f, 0.0f};
		std::vector< uint32_t > sel_ids;
		uint32_t err_id = 0, active_id = 0, hov_id = 0;
	};

	struct SkeletonOpt {
		SkeletonOpt(Skeleton& s, GL::Mesh& face_mesh) : skeleton(s), face_mesh(&face_mesh) {
		}
		SkeletonOpt(Skeleton& s) : skeleton(s) {
		}
		Skeleton& skeleton;
		GL::Mesh* face_mesh = nullptr;
		Mat4 view;
		bool posed = false;
		bool selected_base = false;
		Skeleton::BoneIndex selected_bone = -1U;
		Skeleton::HandleIndex selected_handle = -1U;
		uint32_t first_id = 0;
	};

	void mesh(GL::Mesh& mesh, MeshOpt opt);
	void instances(GL::Instances& inst, GL::Mesh& mesh, MeshOpt opt);
	void lines(const GL::Lines& lines, const Mat4& view, const Mat4& model = Mat4::I,
	           float alpha = 1.0f);

	void begin_outline();
	void end_outline(BBox box);

	void skydome(const Mat4& rotation, Spectrum color, float cosine);
	void skydome(const Mat4& rotation, Spectrum color, float cosine, const GL::Tex2D& tex);

	void sphere(MeshOpt opt);
	void capsule(MeshOpt opt, float height, float rad);
	void capsule(MeshOpt opt, float height, float rad, BBox& box);

	void halfedge_editor(HalfedgeOpt opt);

	struct Skeleton_ID_Map {
		uint32_t base_id = -1U;
		uint32_t mesh_id = -1U;
		uint32_t bone_ids_begin = -1U;
		uint32_t bone_ids_end = -1U;
		uint32_t handle_ids_begin = -1U;
		uint32_t handle_ids_end = -1U;
	};
	Skeleton_ID_Map skeleton(SkeletonOpt opt);

	GLuint saved() const;
	void saved(std::vector<uint8_t>& data) const;
	void save(Gui::Manager& manager, std::shared_ptr<Instance::Camera> camera_instance);

private:
	Renderer(Vec2 dim);
	~Renderer();
	static inline Renderer* data = nullptr;

	GL::Framebuffer framebuffer, id_resolve, save_buffer, save_output, outline_fb;
	GL::Shader mesh_shader, line_shader, inst_shader, dome_shader;
	GL::Mesh _sphere, _cyl, _hemi;

	uint32_t samples;
	Vec2 window_dim;
	GLubyte* id_buffer;

	Mat4 _proj;
};
