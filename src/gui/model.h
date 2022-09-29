#pragma once

#include "widgets.h"
#include "modifiers.h"

#include <SDL.h>
#include <optional>
#include <unordered_map>

#include "../geometry/halfedge.h"
#include "../platform/gl.h"
#include "../scene/scene.h"
#include "../util/viewer.h"
#include "manager.h"

namespace Gui {

class Model {
public:
	Model();

	// Gui view API
	bool keydown(Widgets& widgets, SDL_Keysym key, View_3D& cam);

	void set_mesh(const std::string& name, std::weak_ptr<Halfedge_Mesh> mesh);
	void set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh);
	void invalidate(const std::string& name);

	std::string ui_sidebar(Scene& scene, Undo& undo, Widgets& widgets, View_3D& cam);
	void render(Widgets& widgets, View_3D& cam);

	void dissolve_selected(Undo& undo);
	std::string end_transform(Undo& undo);
	void apply_transform(Widgets& widgets);
	void erase_mesh(const std::string& name);

	void clear_select();
	Vec3 selected_pos();
	void hover(uint32_t id);
	std::string select(Widgets& widgets, uint32_t click, Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods);

	std::tuple<GL::Instances&, GL::Instances&, GL::Instances&> instances();
	std::tuple<GL::Mesh&, GL::Mesh&, GL::Mesh&, GL::Mesh&> meshes();

private:
	bool mesh_expired();
	Halfedge_Mesh& get_mesh();
	void save_old_mesh();
	void load_old_mesh();
	void undo_update_mesh(Undo& undo);

	template<typename T> std::string update_mesh(Undo& undo, std::string const &desc, T&& op);

	void zoom_to(Halfedge_Mesh::ElementRef ref, View_3D& cam);
	void begin_transform();
	bool begin_bevel_or_extrude(std::string& err);

	void rebuild();

	void update_vertex(Halfedge_Mesh::VertexRef vert);
	void vertex_viz(Halfedge_Mesh::VertexRef v, float& size, Mat4& transform);
	void edge_viz(Halfedge_Mesh::EdgeRef e, Mat4& transform);
	void halfedge_viz(Halfedge_Mesh::HalfedgeRef h, Mat4& transform);
	void face_viz(Halfedge_Mesh::FaceRef face, std::vector<GL::Mesh::Vert>& verts,
	              std::vector<GL::Mesh::Index>& idxs, size_t insert_at);

	std::string validate();
	std::string err_msg;

	//selection management:
	// there is a "selected" set and an "active" element.
	// generally operations apply to the "active" element.
	// for some operations, the "selected" set also matters.
	void set_selected(Halfedge_Mesh::ElementRef elem);
	void select_id(uint32_t id, bool toggle = false);
	std::optional<Halfedge_Mesh::ElementRef> active_element();
	std::vector< Halfedge_Mesh::EdgeRef > selected_edges(); //all edges in the selected set. If active element is an edge, it is first.
	//see also: 'clear_select()' above

	uint32_t screen_err_id = 0;
	uint32_t screen_hovered_elem_id = 0;
	uint32_t screen_active_elem_id = 0;
	std::set< uint32_t > screen_selected_elem_ids;

	std::string mesh_name;
	std::variant<std::weak_ptr<Halfedge_Mesh>, std::weak_ptr<Skinned_Mesh>> my_mesh;
	std::variant<Halfedge_Mesh, Skinned_Mesh> old_mesh;

	enum class Bevel { face, edge, vert };
	Bevel beveling;

	struct Transform_Data {
		Vec3 normal;
		std::vector<Vec3> verts;
		Vec3 center;
	};

	Transform_Data trans_begin;
	GL::Instances spheres, cylinders, arrows;
	GL::Mesh face_mesh, vert_mesh, edge_mesh, halfedge_mesh;
	
	Widget_Halfedge_Mesh he_edit_widget;
	Widget_Skinned_Mesh skin_edit_widget;

	Spectrum f_col = Spectrum{1.0f};
	Spectrum v_col = Spectrum{1.0f};
	Spectrum e_col = Spectrum{0.8f};
	Spectrum he_col = Spectrum{0.6f};
	Spectrum err_col = Spectrum{1.0f, 0.0f, 0.0f};

	// This is a kind of bad design and would be un-necessary if we used
	// a halfedge implementation with contiguous iterators. For now this map must
	// be updated (along with the instance data) by rebuild whenever
	// the mesh changes its connectivity.
	struct ElemInfo {
		Halfedge_Mesh::ElementRef ref;
		size_t instance = 0;
	};
	std::unordered_map<uint32_t, ElemInfo> screen_id_to_info;
	std::unordered_map<uint32_t, float> vert_sizes;
	bool needs_rebuild = false;
};

} // namespace Gui
