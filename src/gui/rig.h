
#pragma once

#include <SDL.h>

#include "../pathtracer/tri_mesh.h"
#include "../scene/skeleton.h"
#include "widgets.h"

namespace Gui {

enum class Mode : uint8_t;
class Manager;

class Rig {
public:
    Rig() = default;

	void select(Scene& scene, Widgets& widgets, Undo& undo, uint32_t id, Vec3 cam, Vec2 spos,
	            Vec3 dir);
	void hover(Vec3 cam, Vec2 spos, Vec3 dir);

	void end_transform(Widgets& widgets, Undo& undo);
	void apply_transform(Widgets& widgets);

	void clear_select();
	Vec3 selected_pos();
    void erase_selected(Undo& undo);

    void invalidate(const std::string& name);
    void erase_mesh(const std::string& name);
    void set_mesh(const std::string& name, std::weak_ptr<Skinned_Mesh> mesh);

	void render(Widgets& widgets, View_3D& cam);
	void ui_sidebar(Scene& scene, Undo& undo, Widgets& widgets);

private:
    void rebuild();
    
	bool creating_bone = false;
	bool root_selected = false;
    bool needs_rebuild = false;

	Widget_Skinned_Mesh edit_widget;
	
	Skinned_Mesh old_mesh;
	Vec3 old_pos, old_base, old_ext;
	float old_r = 0.0f;

    std::string mesh_name;
    std::weak_ptr<Skinned_Mesh> my_mesh;
	std::weak_ptr<Bone> selected_bone, new_bone;
	std::weak_ptr<Skeleton::IK_Handle> selected_handle;

	std::unordered_map<uint32_t, std::weak_ptr<Bone>> id_to_bone;
	std::unordered_map<uint32_t, std::weak_ptr<Skeleton::IK_Handle>> id_to_handle;

	bool use_bvh = true, dont_clear_select = false;
	PT::Tri_Mesh mesh_accel;
    GL::Mesh gpu_mesh;
};

} // namespace Gui
