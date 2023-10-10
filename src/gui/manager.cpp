
#include <fstream>
#include <sstream>

#include <imgui/imgui_custom.h>
#include <imgui/imgui_internal.h>
#include <nfd/nfd.h>

#include "manager.h"

#include "../geometry/util.h"
#include "../platform/platform.h"
#include "../platform/renderer.h"
#include "../scene/undo.h"
#include "../test.h"
#include "../scene/io.h"

namespace Gui {

Manager::Manager(Scene& scene, Undo& undo, Animator& animator, Vec2 dim)
	: scene(scene), undo(undo), animator(animator), window_dim(dim), animate(*this),
	  baseplane(1.0f) {

	point_light_mesh = Util::closed_sphere_mesh(0.05f, 1).to_gl();
	directional_light_mesh = Util::arrow_mesh(0.03f, 0.075f, 1.0f).to_gl();
	spot_light_origin_mesh = Util::closed_sphere_mesh(0.1f, 1).to_gl();
	particle_system_mesh = Util::arrow_mesh(0.03f, 0.075f, 1.0f).to_gl();

	const int32_t R = 25;
	for (int32_t i = -R; i <= R; i++) {
		if (i == 0) {
			baseplane.add(Vec3{-R, 0, i}, Vec3{R, 0, i}, Color::red);
			baseplane.add(Vec3{i, 0, -R}, Vec3{i, 0, R}, Color::blue);
			continue;
		}
		baseplane.add(Vec3{i, 0, -R}, Vec3{i, 0, R}, Color::baseplane);
		baseplane.add(Vec3{-R, 0, i}, Vec3{R, 0, i}, Color::baseplane);
	}
}

Render& Manager::get_render() {
	return render;
}

Animate& Manager::get_animate() {
	return animate;
}

Simulate& Manager::get_simulate() {
	return simulate;
}

void Manager::update_dim(Vec2 dim) {
	window_dim = dim;
}

void Manager::shutdown() {
	gpu_mesh_cache.clear();
	gpu_lines_cache.clear();
	gpu_texture_cache.clear();
}

Spectrum Color::axis(Axis a) {
	switch (a) {
	case Axis::X: return red;
	case Axis::Y: return green;
	case Axis::Z: return blue;
	default: assert(false);
	}
	return Spectrum();
}

bool Manager::quit() {
	if (!already_denied_save && n_actions_at_last_save != undo.n_actions()) {
		save_first_shown = true;
		after_save = [this](bool success) {
			already_denied_save = success;
			SDL_Event quit;
			quit.type = SDL_QUIT;
			SDL_PushEvent(&quit);
		};
		return false;
	}
	return true;
}

bool Manager::keydown(SDL_Keysym key, View_3D& gui_cam) {

	if (widgets.is_dragging()) return false;

#ifdef __APPLE__
	Uint16 mod = KMOD_GUI;
	if (key.sym == SDLK_BACKSPACE && key.mod & KMOD_GUI) {
#else
	Uint16 mod = KMOD_CTRL;
	if (key.sym == SDLK_DELETE) {
#endif
		if (mode == Mode::model) {
			model.dissolve_selected(undo);
		} else if (mode == Mode::rig) {
			rig.erase_selected(undo);
		} else {
			erase_selected();
		}
	}

	if (key.mod & mod) {
		switch (key.sym) {
		case SDLK_d: debug_shown = true; return true;
		case SDLK_e: save_scene_as(); return true;
		case SDLK_o: load_scene(); return true;
		case SDLK_s: save_scene_as((save_file.empty() ? nullptr : &save_file)); return true;
		}
	}

	switch (key.sym) {
	case SDLK_m: widgets.active = Widget_Type::move; return true;
	case SDLK_r: widgets.active = Widget_Type::rotate; return true;
	case SDLK_s: widgets.active = Widget_Type::scale; return true;
	case SDLK_f: {
		if (mode == Mode::rig) {
			gui_cam.look_at(Vec3{}, -gui_cam.front() * gui_cam.dist());
		} else if (mode != Mode::model) {
			frame(gui_cam);
		}
	} break;
	}

	switch (mode) {
	case Mode::layout:
	case Mode::simulate:
	case Mode::rig:
	case Mode::render: break;
	case Mode::model: {
		model.keydown(widgets, key, gui_cam);
	} break;
	case Mode::animate: {
		animate.keydown(key);
	} break;
	default: assert(false);
	}
	return false;
}

static bool postfix(const std::string& path, const std::string& type) {
	if (path.length() >= type.length())
		return path.compare(path.length() - type.length(), type.length(), type) == 0;
	return false;
}

bool Manager::save_scene_as(std::string *new_path) {
	if (!new_path) {
		//no file? prompt user:
		char* path = nullptr;
		NFD_SaveDialog(scene_file_types, nullptr, &path);
		if (!path) return false;

		save_file = std::string(path);
		free(path);

		if (postfix(save_file, ".s3d")) {
			//old (binary) format file
		} else if (postfix(save_file, ".js3d")) {
			//new (json) format file
		} else {
			//no extension, make it json-s3d
			save_file += ".js3d";
		}
	} else {
		save_file = *new_path;
	}

	//save the file:
	try {
		save(save_file, scene, animator);
	} catch (std::exception& e) {
		set_error(e.what());
		return false;
	}

	//and update undo info:
	n_actions_at_last_save = undo.n_actions();
	return true;
}

void Manager::new_scene() {

	after_save = [this](bool success) {
		scene = Scene();
		animator = Animator();

		gpu_mesh_cache = std::unordered_map<std::string, GL::Mesh>();
		gpu_lines_cache = std::unordered_map<std::string, GL::Lines>();
		gpu_texture_cache = std::unordered_map<std::string, GL::Tex2D>();

		n_actions_at_last_save = undo.n_actions();
		simulate.build_collision(scene);
		animate.refresh(scene, animator);
	};

	if (n_actions_at_last_save != undo.n_actions()) {
		save_first_shown = true;
		return;
	}

	after_save(true);
}

void Manager::load_scene(std::string *from_path_, Load strategy) {

	bool clear = (strategy == Load::new_scene);

	std::optional< std::string > from_path;
	if (from_path_) from_path = *from_path_;

	after_save = [this,clear,from_path](bool success) {
		if (!success) {
			save_first_shown = true;
			return;
		}

		std::string load_from;

		//no supplied path? ask the user:
		if (from_path) {
			load_from = *from_path;
		} else {
			char* path = nullptr;
			NFD_OpenDialog(scene_file_types, nullptr, &path);
			if (!path) return;
			load_from = std::string(path);
			free(path);
		}

		if (clear) {
			save_file = load_from;
			model.set_mesh({}, std::weak_ptr<Halfedge_Mesh>{});
			rig.set_mesh({}, std::weak_ptr<Skinned_Mesh>{});
			animate.set_mesh({}, std::weak_ptr<Skinned_Mesh>{});
		}

		Scene old_scene(std::move(scene));
		Animator old_animator(std::move(animator));

		try {
			load(load_from, &scene, &animator);
		} catch (std::exception& e) {
			scene = std::move(old_scene);
			animator = std::move(old_animator);
			set_error(e.what());
			warn("Error loading scene: %s", e.what());
			return;
		}

		if (clear) {
			// Avoid rendering conflicts with objects having the same name in previously loaded scenes
			gpu_mesh_cache = std::unordered_map<std::string, GL::Mesh>();
			gpu_lines_cache = std::unordered_map<std::string, GL::Lines>();
			gpu_texture_cache = std::unordered_map<std::string, GL::Tex2D>();
			n_actions_at_last_save = undo.n_actions();
			simulate.build_collision(scene);
			animate.refresh(scene, animator);
		} else {
			old_scene.merge(std::move(scene), animator);
			old_animator.merge(std::move(animator));
			scene = std::move(old_scene);
			animator = std::move(old_animator);
			undo.inc_actions();
		}
	};

	if (!from_path && clear && n_actions_at_last_save != undo.n_actions()) {
		save_first_shown = true;
		return;
	}
	after_save(true);
}

void Manager::to_s3d() {

    std::vector<Indexed_Mesh::Vert> verts;
    std::vector<Indexed_Mesh::Index> idxs;
    std::vector<Vec3> normals;

    // read in lines from file, assign verts and idxs
	char *old_model_path = nullptr;
	NFD_OpenDialog("obj", nullptr, &old_model_path);
	if (!old_model_path) return;

    std::ifstream in_file(std::string(old_model_path), std::ios::in);
    std::string line;
    while (std::getline(in_file, line)) {
        std::istringstream data(line);
        std::string type;
        data >> type;
        if (type == "v") {
            Indexed_Mesh::Vert vert;
            float x, y, z;
            data >> x >> y >> z;
            vert.pos = Vec3(x, y, z);
            vert.uv = Vec2();
            vert.id = static_cast<uint32_t>(verts.size());
            verts.emplace_back(vert);
        } 
        else if (type == "vn") {
            float x, y, z;
            data >> x >> y >> z;
            normals.emplace_back(Vec3(x, y, z));
        }
        else if (type == "f") {
            std::string token;
			std::vector<uint32_t> f_idxs;
            while (data >> token) { // v/vt/vn pair
				std::vector<std::string> v_data;
				std::stringstream v_data_ss(token);
				while (v_data_ss.good()) {
					std::string substr;
					std::getline(v_data_ss, substr, '/');
					v_data.push_back(substr);
				}
				uint32_t vi = static_cast<uint32_t>(std::stoi(v_data[0]));
				// int vt = 0;
				// try { 
				// 	vt = std::stoi(v_data[1]); 
				// } catch (std::exception e) {
				// 	vt = 0;
				// }
				uint32_t vn = static_cast<uint32_t>(std::stoi(v_data[2]));
				verts[vi - 1].norm = normals[vn - 1];
				f_idxs.emplace_back(vi - 1);
            }
			for (size_t i = 1; i + 1 < f_idxs.size(); i++) {
				idxs.emplace_back(static_cast<Indexed_Mesh::Index>(f_idxs[0]));
				idxs.emplace_back(static_cast<Indexed_Mesh::Index>(f_idxs[i]));
				idxs.emplace_back(static_cast<Indexed_Mesh::Index>(f_idxs[i + 1]));
			}
        }
    }

    // verts and idxs -> index mesh -> halfedge mesh
    Indexed_Mesh idx_mesh(std::move(verts), std::move(idxs));
    Halfedge_Mesh he_mesh = Halfedge_Mesh::from_indexed_mesh(idx_mesh);
	Halfedge_Mesh skinned_mesh_data = Halfedge_Mesh::from_indexed_mesh(idx_mesh);
	Skinned_Mesh skinned_mesh;
	skinned_mesh.mesh = std::move(skinned_mesh_data);

    // add new meshes to scene
	scene.create<Halfedge_Mesh>("Imported Mesh", std::move(he_mesh));
	scene.create<Skinned_Mesh>("Imported Skinned Mesh", std::move(skinned_mesh));
}

void Manager::frame(View_3D& gui_cam) {

	if (selected_instance_transform.expired()) return;
	Vec3 center = selected_instance_transform.lock()->local_to_world() * Vec3{};
	Vec3 dir = gui_cam.front() * gui_cam.dist();
	gui_cam.look_at(center, center - dir);
}

void Manager::ui_sidebar(float menu_height, View_3D& gui_cam) {

	using namespace ImGui;

	const ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
	static float anim_height = 0.0f;

	SetNextWindowPos({0.0, menu_height});

	float h_cut = menu_height + (mode == Mode::animate ? anim_height : 0.0f);

	SetNextWindowSizeConstraints({window_dim.x / 4.75f, window_dim.y - h_cut},
	                             {window_dim.x, window_dim.y - h_cut});
	Begin("Menu", nullptr, flags);

	if (mode == Mode::layout) {
		if (Button("Open Scene")) load_scene(nullptr, Load::new_scene);
		if (WrapButton("Save Scene As")) save_scene_as();
		if (WrapButton("Clear")) {
			std::unordered_set<std::string> names = scene.all_names();
			for (auto& name : names) {
				scene.find(name, [&](const std::string& name, auto& resource) {
					undo.erase<std::decay_t<decltype(*resource)>>(name);
				});
			}
			undo.bundle_last(names.size());
		}

		if (Button("Append Objects")) {
			load_scene(nullptr, Load::append);
		}
		if (WrapButton("Create Object")) {
			new_object_shown = true;
			new_object_focus = true;
		}
		if (Button("Import obj")) {
			to_s3d();
		}

		Separator();
	}

	switch (mode) {
	case Mode::layout: {
		if (CollapsingHeader("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen)) {
			ui_scene_graph();
		}
		if (CollapsingHeader("Resources")) {
			ui_resource_list();
		}
		if (selected_object_name) {
			Separator();
			ui_properties(gui_cam);
		}
	} break;
	case Mode::render: {
		if (CollapsingHeader("Scene Graph")) {
			ui_scene_graph();
		}
		if (selected_object_name) {
			Separator();
			ui_properties(gui_cam);
		}
		render.ui_sidebar(*this, undo, scene, gui_cam);
	} break;
	case Mode::model: {
		model.ui_sidebar(scene, undo, widgets, gui_cam);
	} break;
	case Mode::rig: {
		rig.ui_sidebar(scene, undo, widgets);
	} break;
	case Mode::simulate: {
		ui_resource_list_physics();
		if (selected_object_name) {
			Separator();
			ui_properties(gui_cam);
		}
		simulate.ui_sidebar(*this, scene, undo, widgets);
	} break;
	case Mode::animate: {
		if (CollapsingHeader("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen)) {
			ui_scene_graph();
		}
		if (selected_object_name) {
			Separator();
			ui_properties(gui_cam);
		}
		animate.ui_sidebar(undo, gui_cam);
		End();
		SetNextWindowPos({0.0, window_dim.y}, ImGuiCond_Always, {0.0f, 1.0f});
		SetNextWindowSize({window_dim.x, window_dim.y / 4.0f}, ImGuiCond_FirstUseEver);
		SetNextWindowSizeConstraints({window_dim.x, window_dim.y / 4.0f}, window_dim);
		Begin("Timeline", nullptr, flags);
		anim_height = GetWindowHeight();
		animate.ui_timeline(undo, animator, scene, gui_cam, selected_object_name);
	} break;
	default: assert(false);
	}

	End();
}

template<typename I, typename R>
bool Manager::choose_instance(const std::string& label, const std::shared_ptr<I>& instance, std::weak_ptr<R>& resource, bool can_be_null ) {

	using namespace ImGui;
	I old_instance = *instance;

	auto& storage = scene.get_storage<R>();
	const char* sel_name = "[None]";
	for (auto& [name, val] : storage) {
		if constexpr (std::is_same_v<I, R>) {
			if (val == instance) continue;
		}
		if (val == resource.lock()) sel_name = name.c_str();
	}

	bool updated = false;
	bool button_label = !resource.expired();
	auto combo_label = button_label ? "##combo-" + label : label + "##combo";

	if (BeginCombo(combo_label.c_str(), sel_name)) {
		for (auto& [name, val] : storage) {
			if constexpr (std::is_same_v<I, R>) {
				if (val == instance) continue;
			}
			if (Selectable(name.c_str(), val == resource.lock())) {
				resource = val;
				undo.update<I>(instance, old_instance);
				updated = true;
			}
		}
		if (Selectable("New...", false)) {
			resource = scene.get<R>(undo.create(scene.get_type_name<R>(), R{}));
			undo.update<I>(instance, old_instance);
			undo.bundle_last(2);
			updated = true;
		}
		if (can_be_null && Selectable("[None]", false)) {
			resource.reset();
			undo.update<I>(instance, old_instance);
			updated = true;
		}
		EndCombo();
	}

	if (button_label) {
		SameLine();
		if (Button(label.c_str())) {
			if (auto name = scene.name<R>(resource)) {
				set_select(name.value());
			}
		}
	}

	return updated;
}

void Manager::erase_selected() {

	if (!selected_object_name) return;
	auto name = selected_object_name.value();

	model.erase_mesh(name);
	rig.erase_mesh(name);
	animate.erase_mesh(name);

	auto _selected = scene.get(name);
	if (!_selected) return;
	auto selected = _selected.value();

	uint32_t n = 0;

	auto remove_material = [&](const std::shared_ptr<Material>& material, auto type) {
		using I = decltype(type);
		for (auto& [_, inst] : scene.get_storage<I>()) {
			if (inst->material.lock() != material) continue;
			I i = *inst;
			inst->material.reset();
			undo.update<I>(inst, i);
			n++;
		}
	};
	auto remove_mesh = [&](const std::shared_ptr<Halfedge_Mesh>& mesh, auto type) {
		using I = decltype(type);
		for (auto& [_, inst] : scene.get_storage<I>()) {
			if (inst->mesh.lock() != mesh) continue;
			I i = *inst;
			inst->mesh.reset();
			undo.update<I>(inst, i);
			n++;
		}
	};

	std::visit(overloaded{
				   [&](const std::weak_ptr<Transform>& _transform) {
					   auto transform = _transform.lock();
					   for (auto& [_, inst] : scene.transforms) {
						   if (inst->parent.lock() != transform) continue;
						   Transform t = *inst;
						   inst->parent.reset();
						   undo.update<Transform>(inst, t);
						   n++;
					   }
					   scene.for_each_instance([&](const std::string& name, auto& inst) {
						   using I = std::decay_t<decltype(*inst)>;
						   if (inst->transform.lock() != transform) return;
						   I i = *inst;
						   inst->transform.reset();
						   undo.update<I>(inst, i);
						   n++;
					   });
					   undo.erase<Transform>(name);
				   },
				   [&](const std::weak_ptr<Material>& _material) {
					   auto material = _material.lock();
					   remove_material(material, Instance::Mesh{});
					   remove_material(material, Instance::Skinned_Mesh{});
					   remove_material(material, Instance::Shape{});
					   remove_material(material, Instance::Particles{});
				   },
				   [&](const std::weak_ptr<Halfedge_Mesh>& _halfedge_mesh) {
					   auto mesh = _halfedge_mesh.lock();
					   remove_mesh(mesh, Instance::Mesh{});
					   remove_mesh(mesh, Instance::Particles{});
				   },
				   [&](const std::weak_ptr<Texture>& _texture) {
					   auto texture = _texture.lock();
					   for (auto& [_, material] : scene.materials) {
						   bool changed = false;
						   Material m = *material;
						   material->for_each([&](std::weak_ptr<Texture>& tex) {
							   if (tex.lock() != texture) return;
							   tex.reset();
							   changed = true;
						   });
						   if (changed) {
							   undo.update<Material>(material, m);
							   n++;
						   }
					   }
				   },
				   [&](const std::weak_ptr<Camera>& _camera) {
					   auto camera = _camera.lock();
					   for (auto& [_, inst] : scene.instances.cameras) {
						   if (inst->camera.lock() != camera) continue;
						   Instance::Camera c = *inst;
						   inst->camera.reset();
						   undo.update<Instance::Camera>(inst, c);
						   n++;
					   }
				   },
				   [&](const std::weak_ptr<Delta_Light>& _delta_light) {
					   auto delta_light = _delta_light.lock();
					   for (auto& [_, inst] : scene.instances.delta_lights) {
						   if (inst->light.lock() != delta_light) continue;
						   Instance::Delta_Light d = *inst;
						   inst->light.reset();
						   undo.update<Instance::Delta_Light>(inst, d);
						   n++;
					   }
				   },
				   [&](const std::weak_ptr<Environment_Light>& _environment_light) {
					   auto environment_light = _environment_light.lock();
					   for (auto& [_, inst] : scene.instances.env_lights) {
						   if (inst->light.lock() != environment_light) continue;
						   Instance::Environment_Light e = *inst;
						   inst->light.reset();
						   undo.update<Instance::Environment_Light>(inst, e);
						   n++;
					   }
				   },
				   [&](const std::weak_ptr<Shape>& _shape) {
					   auto shape = _shape.lock();
					   for (auto& [_, inst] : scene.instances.shapes) {
						   if (inst->shape.lock() != shape) continue;
						   Instance::Shape s = *inst;
						   inst->shape.reset();
						   undo.update<Instance::Shape>(inst, s);
						   n++;
					   }
				   },
				   [&](const std::weak_ptr<Particles>& _particles) {
					   auto particles = _particles.lock();
					   for (auto& [_, inst] : scene.instances.particles) {
						   if (inst->particles.lock() != particles) continue;
						   Instance::Particles p = *inst;
						   inst->particles.reset();
						   undo.update<Instance::Particles>(inst, p);
						   n++;
					   }
				   },
				   [&](const std::weak_ptr<Skinned_Mesh>& _skinned_mesh) {
					   auto skinned_mesh = _skinned_mesh.lock();
					   for (auto& [_, inst] : scene.instances.skinned_meshes) {
						   if (inst->mesh.lock() != skinned_mesh) continue;
						   Instance::Skinned_Mesh s = *inst;
						   inst->mesh.reset();
						   undo.update<Instance::Skinned_Mesh>(inst, s);
						   n++;
					   }
				   },
				   [&](const std::weak_ptr<Instance::Skinned_Mesh>& _skinned_inst) {
					   auto skinned_inst = _skinned_inst.lock();
					   if (!skinned_inst->mesh.expired()) {
						   animate.erase_mesh(scene.name<Skinned_Mesh>(skinned_inst->mesh).value());
					   }
				   },
				   [&](const auto& inst) {},
			   },
	           selected);

	scene.find(name, [&](const std::string& name, auto& resource) {
		undo.erase<std::decay_t<decltype(*resource)>>(name);
		n++;
	});

	if (n > 1) undo.bundle_last(n);
	selected_object_name.reset();
}

void Manager::edit_camera(std::shared_ptr<Instance::Camera> inst, View_3D& gui_cam) {

	using namespace ImGui;

	std::unordered_map<std::shared_ptr<Camera>, std::string> camera_names;
	for (auto& [name, camera] : scene.cameras) {
		camera_names[camera] = name;
	}

	choose_instance("Camera", inst, inst->camera);
	if (camera_names.count(inst->camera.lock())) {

		auto camera = inst->camera.lock();
		auto cam_name = camera_names.at(camera);

		camera_widget.ui(undo, cam_name, inst->camera);

		if (Button("Set Resolution")) {
			Camera old = *camera;
			camera->film.width = static_cast<uint32_t>(window_dim.x);
			camera->film.height = static_cast<uint32_t>(window_dim.y);
			camera->aspect_ratio = window_dim.x / window_dim.y;
			undo.update_cached<Camera>(cam_name, camera, old);
		}
		if (!inst->transform.expired()) {
			auto transform = inst->transform.lock();
			if (WrapButton("Move to View")) {
				Mat4 v = gui_cam.get_view().inverse();
				Transform old = *transform;
				transform->translation = v * Vec3{0.0f};
				transform->rotation = Quat::euler(v.to_euler());
				undo.update<Transform>(transform, old);
			}
		}
	}
}

void Manager::ui_properties(View_3D& gui_cam) {

	using namespace ImGui;

	if (!selected_object_name) return;
	auto _selected = scene.get(selected_object_name.value());

	if (!_selected) return;
	auto selected = _selected.value();

	std::unordered_map<std::shared_ptr<Halfedge_Mesh>, std::string> mesh_names;
	std::unordered_map<std::shared_ptr<Skinned_Mesh>, std::string> skinned_mesh_names;
	std::unordered_map<std::shared_ptr<Shape>, std::string> shape_names;
	std::unordered_map<std::shared_ptr<Delta_Light>, std::string> delta_light_names;
	{
		for (auto& [name, mesh] : scene.meshes) {
			mesh_names[mesh] = name;
		}
		for (auto& [name, mesh] : scene.skinned_meshes) {
			skinned_mesh_names[mesh] = name;
		}
		for (auto& [name, shape] : scene.shapes) {
			shape_names[shape] = name;
		}
		for (auto& [name, delta_light] : scene.delta_lights) {
			delta_light_names[delta_light] = name;
		}
	}

	auto edit_transform = [&](auto& inst) {
		if (BeginTabItem("Transform##tab")) {
			choose_instance("Transform", inst, inst->transform);
			choose_instance("Parent", inst->transform.lock(), inst->transform.lock()->parent, true);
			widgets.active = transform_widget.ui(widgets.active, undo, inst->transform);
			EndTabItem();
		}
	};
	auto edit_mesh = [&](auto& inst) {
		if (BeginTabItem("Mesh##tab")) {
			choose_instance("Mesh", inst, inst->mesh);
			if (mesh_names.count(inst->mesh.lock())) {
				auto mesh_name = mesh_names.at(inst->mesh.lock());
				mode = halfedge_mesh_widget.ui(mode, mesh_name, undo, inst->mesh);
				if (mode == Mode::model) {
					model.set_mesh(mesh_name, inst->mesh);
				}
			}
			EndTabItem();
		}
	};
	auto edit_skinned_mesh = [&](auto& inst) {
		if (BeginTabItem("Mesh##tab")) {
			choose_instance("Mesh", inst, inst->mesh);
			if (skinned_mesh_names.count(inst->mesh.lock())) {
				auto mesh_name = skinned_mesh_names.at(inst->mesh.lock());
				mode = skinned_mesh_widget.ui(mode, mesh_name, undo, inst->mesh);
				if (mode == Mode::model) {
					model.set_mesh(mesh_name, inst->mesh);
				} else if (mode == Mode::rig) {
					rig.set_mesh(mesh_name, inst->mesh);
				} else if (mode == Mode::animate) {
					animate.set_mesh(mesh_name, inst->mesh);
				}
			}
			EndTabItem();
		}
	};
	auto edit_material = [&](auto& inst) {
		if (BeginTabItem("Material##tab")) {
			choose_instance("Material", inst, inst->material);
			material_widget.ui(*this, undo, inst->material);
			EndTabItem();
		}
	};
	auto edit_shape = [&](auto& inst) {
		if (BeginTabItem("Shape##tab")) {
			choose_instance("Shape", inst, inst->shape);
			if (shape_names.count(inst->shape.lock())) {
				auto shape_name = shape_names.at(inst->shape.lock());
				shape_widget.ui(shape_name, undo, inst->shape);
			}
			EndTabItem();
		}
	};
	auto edit_delta_light = [&](auto& inst) {
		if (BeginTabItem("Light##tab")) {
			choose_instance("Light", inst, inst->light);
			if (delta_light_names.count(inst->light.lock())) {
				auto delta_light_name = delta_light_names.at(inst->light.lock());
				delta_light_widget.ui(delta_light_name, *this, undo, inst->light);
			}
			EndTabItem();
		}
	};
	auto edit_environment_light = [&](auto& inst) {
		if (BeginTabItem("Light##tab")) {
			choose_instance("Light", inst, inst->light);
			environment_light_widget.ui(*this, undo, inst->light);
			EndTabItem();
		}
	};
	auto edit_particles = [&](auto& inst) {
		if (BeginTabItem("Particles##tab")) {
			choose_instance("Particles", inst, inst->particles);
			particles_widget.ui(undo, inst->particles);
			EndTabItem();
		}
	};

	auto edit_sim_settings = [&](auto& inst) {
		using I = std::decay_t<decltype(*inst)>;
		{
			I old = *inst;
			bool changed = Checkbox("Visible", &inst->settings.visible);
			changed = changed || Checkbox("Wireframe", &inst->settings.wireframe);
			if (changed) undo.update<I>(inst, std::move(old));
		}

		I old = *inst;
		bool f = false;
		bool* sim = inst->particles.expired() ? &f : &inst->settings.simulate_here;
		if (Checkbox("Simulate Here", sim)) {
			uint32_t n = 0;
			if (inst->settings.simulate_here) {
				for (auto& [_, inst2] : scene.instances.particles) {
					if (inst != inst2 && inst2->particles.lock() == inst->particles.lock()) {
						I old2 = *inst2;
						inst2->settings.simulate_here = false;
						undo.update<I>(inst2, std::move(old2));
						n++;
					}
				}
			}
			if (old.settings.simulate_here != inst->settings.simulate_here) {
				undo.update<I>(inst, std::move(old));
				n++;
			}
			undo.bundle_last(n);
		}
	};
	auto edit_geom_settings = [&](auto& inst) {
		using I = std::decay_t<decltype(*inst)>;
		I old = *inst;
		bool changed = Checkbox("Visible", &inst->settings.visible);

		// Draw style updates
		int draw_style = int(inst->settings.draw_style);
		const char* label = nullptr;
		switch (draw_style) {
			case int(DrawStyle::Wireframe): label = "Wireframe"; break;
			case int(DrawStyle::Flat): label = "Flat"; break;
			case int(DrawStyle::Smooth): label = "Smooth"; break;
			case int(DrawStyle::Correct): label = "Correct"; break;
			default: die("Unknown shape type");
		}

		if (BeginCombo("Draw Style", label)) {
			if (Selectable("Wireframe")) draw_style = int(DrawStyle::Wireframe);
			if (Selectable("Flat")) draw_style = int(DrawStyle::Flat);
			if (Selectable("Smooth")) draw_style = int(DrawStyle::Smooth);
			if (Selectable("Correct")) draw_style = int(DrawStyle::Correct);
			EndCombo();
		}

		if (draw_style != int(inst->settings.draw_style)) {
			changed = true;
			inst->settings.draw_style = DrawStyle(draw_style);
		}

		// Blend style updates
		int blend_style = int(inst->settings.blend_style);
		label = nullptr;
		switch (blend_style) {
			case int(BlendStyle::Replace): label = "Blend Replace"; break;
			case int(BlendStyle::Add): label = "Blend Add"; break;
			case int(BlendStyle::Over): label = "Blend Over"; break;
			default: die("Unknown shape type");
		}

		if (BeginCombo("Blend Style", label)) {
			if (Selectable("Blend Replace")) blend_style = int(BlendStyle::Replace);
			if (Selectable("Blend Add")) blend_style = int(BlendStyle::Add);
			if (Selectable("Blend Over")) blend_style = int(BlendStyle::Over);
			EndCombo();
		}

		if (blend_style != int(inst->settings.blend_style)) {
			changed = true;
			inst->settings.blend_style = BlendStyle(blend_style);
		}

		// Depth style updates
		int depth_style = int(inst->settings.depth_style);
		label = nullptr;
		switch (depth_style) {
			case int(DepthStyle::Always): label = "Depth Always"; break;
			case int(DepthStyle::Never): label = "Depth Never"; break;
			case int(DepthStyle::Less): label = "Depth Less"; break;
			default: die("Unknown shape type");
		}

		if (BeginCombo("Depth Style", label)) {
			if (Selectable("Depth Always")) depth_style = int(DepthStyle::Always);
			if (Selectable("Depth Never")) depth_style = int(DepthStyle::Never);
			if (Selectable("Depth Less")) depth_style = int(DepthStyle::Less);
			EndCombo();
		}

		if (depth_style != int(inst->settings.depth_style)) {
			changed = true;
			inst->settings.depth_style = DepthStyle(depth_style);
		}

		// do update if any of the styles changed
		if (changed) undo.update<I>(inst, old);
	};
	auto edit_light_settings = [&](auto& inst) {
		using I = std::decay_t<decltype(*inst)>;
		I old = *inst;
		bool changed = Checkbox("Visible", &inst->settings.visible);
		if (changed) undo.update<I>(inst, old);
	};

	if (BeginTabBar("Properties##tabs")) {

		if (BeginTabItem("Object")) {

			std::string name = selected_object_name.value();
			std::string old_name = name;

			static char name_buf[256] = {};
			Platform::strcpy(name_buf, name.c_str(), sizeof(name_buf));

			InputText("Name", name_buf, sizeof(name_buf));
			if (IsItemDeactivatedAfterEdit()) {
				name = std::string(name_buf);
				undo.rename(old_name, name);
				set_select(name);
			}

			std::visit(overloaded{
						   [&](std::weak_ptr<Instance::Mesh>& _mesh) {
							   auto mesh = _mesh.lock();
							   edit_geom_settings(mesh);
						   },
						   [&](std::weak_ptr<Instance::Skinned_Mesh>& _skinned_mesh) {
							   auto skinned_mesh = _skinned_mesh.lock();
							   edit_geom_settings(skinned_mesh);
						   },
						   [&](std::weak_ptr<Instance::Shape>& _shape) {
							   auto shape = _shape.lock();
							   edit_geom_settings(shape);
						   },
						   [&](std::weak_ptr<Instance::Delta_Light>& _delta_light) {
							   auto delta_light = _delta_light.lock();
							   edit_light_settings(delta_light);
						   },
						   [&](std::weak_ptr<Instance::Environment_Light>& _environment_light) {
							   auto environment_light = _environment_light.lock();
							   edit_light_settings(environment_light);
						   },
						   [&](std::weak_ptr<Instance::Particles>& _particles) {
							   auto particles = _particles.lock();
							   edit_sim_settings(particles);
						   },
						   [&](const auto&) {},
					   },
			           selected);

			if (Button("Delete")) {
				erase_selected();
			}
			EndTabItem();
		}

		std::visit(
			overloaded{
				[&](const std::weak_ptr<Instance::Mesh>& _mesh) {
					auto mesh = _mesh.lock();
					edit_transform(mesh);
					edit_mesh(mesh);
					edit_material(mesh);
				},
				[&](const std::weak_ptr<Instance::Skinned_Mesh>& _skinned_mesh) {
					auto skinned_mesh = _skinned_mesh.lock();
					edit_transform(skinned_mesh);
					edit_skinned_mesh(skinned_mesh);
					edit_material(skinned_mesh);
				},
				[&](const std::weak_ptr<Instance::Shape>& _shape) {
					auto shape = _shape.lock();
					edit_transform(shape);
					edit_shape(shape);
					edit_material(shape);
				},
				[&](const std::weak_ptr<Instance::Delta_Light>& _delta_light) {
					auto delta_light = _delta_light.lock();
					edit_transform(delta_light);
					edit_delta_light(delta_light);
				},
				[&](const std::weak_ptr<Instance::Environment_Light>& _environment_light) {
					auto environment_light = _environment_light.lock();
					edit_transform(environment_light);
					edit_environment_light(environment_light);
				},
				[&](const std::weak_ptr<Instance::Particles>& _particles) {
					auto particles = _particles.lock();
					edit_transform(particles);
					edit_mesh(particles);
					edit_particles(particles);
					edit_material(particles);
				},
				[&](const std::weak_ptr<Instance::Camera>& _camera) {
					auto camera = _camera.lock();
					edit_transform(camera);
					if (BeginTabItem("Camera##tab")) {
						edit_camera(camera, gui_cam);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Transform>& _transform) {
					if (BeginTabItem("Transform")) {
						auto transform = _transform.lock();
						choose_instance("Parent", transform, transform->parent, true);
						widgets.active = transform_widget.ui(widgets.active, undo, transform);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Camera>& camera) {
					if (BeginTabItem("Camera")) {
						camera_widget.ui(undo, selected_object_name.value(), camera);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Delta_Light>& delta_light) {
					if (BeginTabItem("Light")) {
						delta_light_widget.ui(selected_object_name.value(), *this, undo,
				                              delta_light);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Environment_Light>& environment_light) {
					if (BeginTabItem("Light")) {
						environment_light_widget.ui(*this, undo, environment_light);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Material>& material) {
					if (BeginTabItem("Material")) {
						material_widget.ui(*this, undo, material);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Shape>& shape) {
					if (BeginTabItem("Shape")) {
						shape_widget.ui(selected_object_name.value(), undo, shape);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Particles>& particles) {
					if (BeginTabItem("Particles")) {
						particles_widget.ui(undo, particles);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Texture>& texture) {
					if (BeginTabItem("Texture")) {
						texture_widget.ui(selected_object_name.value(), *this, undo, texture);
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Skinned_Mesh>& skinned_mesh) {
					if (BeginTabItem("Mesh")) {
						mode = skinned_mesh_widget.ui(mode, selected_object_name.value(), undo,
				                                      skinned_mesh);
						if (mode == Mode::model) {
							model.set_mesh(selected_object_name.value(), skinned_mesh);
						} else if (mode == Mode::rig) {
							rig.set_mesh(selected_object_name.value(), skinned_mesh);
						} else if (mode == Mode::animate) {
							animate.set_mesh(selected_object_name.value(), skinned_mesh);
						}
						EndTabItem();
					}
				},
				[&](const std::weak_ptr<Halfedge_Mesh>& halfedge_mesh) {
					if (BeginTabItem("Mesh")) {
						mode = halfedge_mesh_widget.ui(mode, selected_object_name.value(), undo,
				                                       halfedge_mesh);
						if (mode == Mode::model) {
							model.set_mesh(selected_object_name.value(), halfedge_mesh);
						}
						EndTabItem();
					}
				},
			},
			selected);

		EndTabBar();
	}
}

void Manager::ui_resource_list_physics() {

	using namespace ImGui;

	std::optional<std::string> node_clicked;

	auto bullet = [&](const std::string& name) {
		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Bullet |
		                                ImGuiTreeNodeFlags_NoTreePushOnOpen |
		                                ImGuiTreeNodeFlags_SpanAvailWidth;
		bool is_selected = name == selected_object_name;
		if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

		TreeNodeEx(name.c_str(), node_flags, "%s", name.c_str());
		if (IsItemClicked()) node_clicked = name;
	};

	if (CollapsingHeader("Physics Objects")) {
		for (auto& [name, _] : scene.particles) {
			bullet(name);
		}
	}
	if (CollapsingHeader("Physics Instances")) {
		for (auto& [name, _] : scene.instances.particles) {
			bullet(name);
		}
	}

	if (node_clicked) set_select(node_clicked.value());
}

void Manager::ui_resource_list() {

	using namespace ImGui;

	std::optional<std::string> node_clicked;

	auto bullet = [&](const std::string& name) {
		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Bullet |
		                                ImGuiTreeNodeFlags_NoTreePushOnOpen |
		                                ImGuiTreeNodeFlags_SpanAvailWidth;
		bool is_selected = name == selected_object_name;
		if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

		TreeNodeEx(name.c_str(), node_flags, "%s", name.c_str());
		if (IsItemClicked()) node_clicked = name;
	};

	if (TreeNode("Shapes")) {
		for (auto& [name, _] : scene.shapes) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Meshes")) {
		for (auto& [name, _] : scene.meshes) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Skinned Meshes")) {
		for (auto& [name, _] : scene.skinned_meshes) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Particles")) {
		for (auto& [name, _] : scene.particles) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Textures")) {
		for (auto& [name, _] : scene.textures) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Materials")) {
		for (auto& [name, _] : scene.materials) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Delta Lights")) {
		for (auto& [name, _] : scene.delta_lights) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Environment lights")) {
		for (auto& [name, _] : scene.env_lights) {
			bullet(name);
		}
		TreePop();
	}
	if (TreeNode("Cameras")) {
		for (auto& [name, _] : scene.cameras) {
			bullet(name);
		}
		TreePop();
	}

	if (node_clicked) set_select(node_clicked.value());
}

void Manager::ui_scene_graph() {

	using namespace ImGui;

	std::optional<std::string> node_clicked;
	ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow |
	                                ImGuiTreeNodeFlags_OpenOnDoubleClick |
	                                ImGuiTreeNodeFlags_SpanAvailWidth;

	using Child = std::variant<std::shared_ptr<Transform>, std::string>;

	std::unordered_map<std::shared_ptr<Transform>, std::string> transform_names;
	std::map<std::string, std::vector<Child>> heirarchy;
	std::set<std::string> orphan_instances;
	std::unordered_set<std::shared_ptr<Transform>> transforms_on_selected_path;
	{
		for (auto& [name, transform] : scene.transforms) {
			transform_names[transform] = name;
		}
		for (auto& [name, transform] : scene.transforms) {
			if (!transform->parent.expired()) {
				heirarchy[transform_names.at(transform->parent.lock())].push_back(transform);
			}
		}
		scene.for_each_instance([&](const std::string& name, auto& inst) {
			if (inst->transform.expired()) {
				orphan_instances.insert(name);
				return;
			}
			heirarchy[transform_names.at(inst->transform.lock())].push_back(name);
		});

		auto selected = selected_instance_transform;
		bool selected_is_transform = false;
		if (selected_object_name) {
			selected_is_transform = scene.transforms.count(selected_object_name.value()) > 0;
			scene.find_instance(
				selected_object_name.value(),
				[&](const std::string& name, auto& inst) { selected = inst->transform; });
		}
		if (selected_is_transform && !selected.expired()) {
			selected = selected.lock()->parent;
		}
		while (!selected.expired()) {
			transforms_on_selected_path.insert(selected.lock());
			selected = selected.lock()->parent;
		}
	}

	auto ui_instance = [&](const std::string& name) {
		ImGuiTreeNodeFlags node_flags =
			base_flags | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		bool is_selected = name == selected_object_name;
		if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

		TreeNodeEx(name.c_str(), node_flags, "%s", name.c_str());
		if (IsItemClicked()) node_clicked = name;

		if (BeginDragDropSource()) {
			SetDragDropPayload("INSTANCE", name.c_str(), name.length());
			Text("%s", name.c_str());
			EndDragDropSource();
		}
	};

	std::function<void(const std::string&, const std::shared_ptr<Transform>&)> ui_transform;
	ui_transform = [&](const std::string& name, const std::shared_ptr<Transform>& transform) {
		ImGuiTreeNodeFlags node_flags = base_flags;
		bool is_selected = name == selected_object_name;
		if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

		if (transforms_on_selected_path.count(transform)) SetNextItemOpen(true);

		bool node_open = TreeNodeEx(name.c_str(), node_flags, "%s", name.c_str());
		if (IsItemClicked()) node_clicked = name;

		if (BeginDragDropSource()) {
			SetDragDropPayload("TRANSFORM", name.c_str(), name.length());
			Text("%s", name.c_str());
			EndDragDropSource();
		}
		if (BeginDragDropTarget()) {
			if (auto payload = AcceptDragDropPayload("TRANSFORM")) {
				std::string child_name(static_cast<const char*>(payload->Data), payload->DataSize);
				auto& child = scene.transforms.at(child_name);
				std::unordered_set<std::shared_ptr<Transform>> path_to_root;
				{
					auto iter = transform;
					while (iter) {
						path_to_root.insert(iter);
						iter = iter->parent.lock();
					}
				}
				if (!path_to_root.count(child)) {
					Transform old = *child;
					child->parent = transform;
					undo.update<Transform>(child, old);
				}
			}
			if (auto payload = AcceptDragDropPayload("INSTANCE")) {
				std::string child_name(static_cast<const char*>(payload->Data), payload->DataSize);
				scene.find_instance(child_name, [&](const std::string& name, auto& inst) {
					using I = std::decay_t<decltype(*inst)>;
					I old = *inst;
					inst->transform = transform;
					undo.update<I>(inst, old);
				});
			}
			EndDragDropTarget();
		}

		if (node_open) {
			auto children = heirarchy.find(name);
			if (children != heirarchy.end()) {
				for (auto& child : children->second) {
					std::visit(overloaded{
								   [&](std::shared_ptr<Transform>& child) {
									   ui_transform(transform_names.at(child), child);
								   },
								   [&](const std::string& child) { ui_instance(child); },
							   },
					           child);
				}
			}
			TreePop();
		}
	};

	std::set<std::string> transform_names_ordered;
	for (auto& [name, _] : scene.transforms) {
		transform_names_ordered.insert(name);
	}
	for (const auto& name : transform_names_ordered) {
		auto transform = scene.get<Transform>(name).lock();
		if (transform->parent.expired()) {
			ui_transform(name, transform);
		}
	}
	for (const auto& name : orphan_instances) {
		ui_instance(name);
	}

	if (node_clicked) set_select(node_clicked.value());
}

void Manager::set_error(std::string msg) {
	if (msg.empty()) return;
	error_msg = msg;
	error_shown = true;
}

std::optional<std::string> Manager::choose_image() {
	std::optional<std::string> ret;
	char* path = nullptr;
	NFD_OpenDialog(image_file_types, nullptr, &path);
	if (path) {
		ret = std::string{path};
		free(path);
	}
	return ret;
}

void Manager::edit_texture(std::weak_ptr<Texture> tex) {
	if (auto name = scene.name<Texture>(tex)) {
		texture_widget.ui(name.value(), *this, undo, tex);
	}
}

void Manager::render_image(const std::string& tex_name, Vec2 size) {
	auto entry = gpu_texture_cache.find(tex_name);
	if (entry != gpu_texture_cache.end()) {
		ImGui::Image((ImTextureID)(uint64_t)entry->second.get_id(), size, Vec2{0.0f, 1.0f}, Vec2{1.0f, 0.0f});
	}
}

void Manager::render_ui(View_3D& gui_cam) {

	float height = ui_menu();
	ui_sidebar(height, gui_cam);
	ui_error();
	ui_settings();
	ui_savefirst();
	ui_new_object();
	set_error(animate.pump_output(scene, animator));
}

void Manager::ui_new_object() {

	using namespace ImGui;

	if (!new_object_shown) return;
	if (new_object_focus) {
		SetNextWindowFocus();
		new_object_focus = false;
	}

	SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
	Begin("Create Object", &new_object_shown, ImGuiWindowFlags_NoSavedSettings);

	auto default_material = [this]() {
		std::shared_ptr< Material > material = scene.get< Material >("Material").lock();
		if (!material) {
			material = scene.get<Material>(undo.create("Material", Material{scene.get<Texture>(undo.create("Texture", Texture{}))})).lock();
			assert(material);
		}
		return material;
	};

	if (Button("Mesh Instance")) {
		set_select(undo.create<Instance::Mesh>("Mesh Instance",
			Instance::Mesh{
				scene.get<Transform>(undo.create("Transform", Transform{})),
				scene.get<Halfedge_Mesh>(undo.create("Mesh", Halfedge_Mesh::cube(1.0f))),
				default_material(),
				Instance::Geometry_Settings{}
			}
		));
	}
	if (WrapButton("Skinned Mesh Instance"))
		set_select(undo.create<Instance::Skinned_Mesh>("Skinned Mesh Instance", Instance::Skinned_Mesh{scene.get<Transform>(undo.create("Transform", Transform{})), 
																										scene.get<Skinned_Mesh>(undo.create("Skinned Mesh", Skinned_Mesh{})), 
																										default_material(),
																										Instance::Geometry_Settings{}}));
	if (WrapButton("Shape Instance")) 
		set_select(undo.create<Instance::Shape>("Shape Instance", Instance::Shape{scene.get<Transform>(undo.create("Transform", Transform{})), 
																					scene.get<Shape>(undo.create("Shape", Shape{})), 
																					default_material(),
																					Instance::Geometry_Settings{}}));
	if (WrapButton("Delta Light Instance"))
		set_select(undo.create<Instance::Delta_Light>("Delta Light Instance", Instance::Delta_Light{scene.get<Transform>(undo.create("Transform", Transform{})), 
																									scene.get<Delta_Light>(undo.create("Delta Light", Delta_Light{})), 
																									Instance::Light_Settings{}}));
	if (WrapButton("Environment Light Instance"))
		set_select(undo.create<Instance::Environment_Light>("Env Light Instance", Instance::Environment_Light{scene.get<Transform>(undo.create("Transform", Transform{})), 
																												scene.get<Environment_Light>(undo.create("Env Light", Environment_Light{})), 
																												Instance::Light_Settings{}}));
	if (WrapButton("Particles Instance"))
		set_select(undo.create<Instance::Particles>("Particles Instance", Instance::Particles{scene.get<Transform>(undo.create("Transform", Transform{})), 
																								scene.get<Halfedge_Mesh>(undo.create("Mesh", Halfedge_Mesh{})), 
																								default_material(),
																								scene.get<Particles>(undo.create("Particles", Particles{})),
																								Instance::Simulate_Settings{}}));
	if (WrapButton("Camera Instance")) 
		set_select(undo.create<Instance::Camera>("Camera Instance", Instance::Camera{scene.get<Transform>(undo.create("Transform", Transform{})), 
																						scene.get<Camera>(undo.create("Camera", Camera{}))}));

	Separator();

	if (Button("Transform")) set_select(undo.create<Transform>("Transform", Transform{}));
	if (WrapButton("Shape")) undo.create<Shape>("Shape", Shape{});
	if (WrapButton("Mesh")) undo.create<Halfedge_Mesh>("Mesh", Halfedge_Mesh{});
	if (WrapButton("Skinned Mesh")) undo.create<Skinned_Mesh>("Skinned Mesh", Skinned_Mesh{});
	if (WrapButton("Particles")) undo.create<Particles>("Particles", Particles{});
	if (WrapButton("Texture")) undo.create<Texture>("Texture", Texture{});
	if (WrapButton("Material")) undo.create<Material>("Material", Material{scene.get<Texture>(undo.create("Texture", Texture{}))});
	if (WrapButton("Delta Light")) undo.create<Delta_Light>("Delta Light", Delta_Light{});
	if (WrapButton("Environment Light")) undo.create<Environment_Light>("Env Light", Environment_Light{});
	if (WrapButton("Camera")) undo.create<Camera>("Camera", Camera{});

	End();
}

void Manager::ui_savefirst() {

	if (!save_first_shown) return;

	Vec2 center = window_dim / 2.0f;
	ImGui::SetNextWindowPos(Vec2{center.x, center.y}, 0, Vec2{0.5f, 0.5f});
	ImGui::Begin("Save Changes?", &save_first_shown,
	             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize |
	                 ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::Button("Yes")) {
		save_first_shown = false;
		after_save(save_scene_as((save_file.empty() ? nullptr : &save_file)));
	}
	ImGui::SameLine();
	if (ImGui::Button("No")) {
		save_first_shown = false;
		after_save(true);
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		save_first_shown = false;
		after_save = {};
	}
	ImGui::End();
}

void Manager::ui_settings() {

	using namespace ImGui;

	if (!settings_shown) return;

	Begin("Settings", &settings_shown,
	      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

	Text("Scene Importer");

	Separator();
	Text("UI Renderer");
	Combo("Multisampling", &samples.samples, GL::Sample_Count_Names, samples.n_options());

	if (Button("Apply")) {
		Renderer::get().set_samples(samples.n_samples());
	}

	Separator();
	Text("GPU: %s", GL::renderer().c_str());
	Text("OpenGL: %s", GL::version().c_str());

	End();
}

void Manager::ui_error() {

	using namespace ImGui;

	if (!error_shown) return;
	Vec2 center = window_dim / 2.0f;
	SetNextWindowPos(Vec2{center.x, center.y}, 0, Vec2{0.5f, 0.5f});
	Begin("Errors", &error_shown,
	      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
	          ImGuiWindowFlags_NoResize);
	if (!error_msg.empty()) Text("%s", error_msg.c_str());
	if (Button("Close")) {
		error_shown = false;
	}
	End();
}

float Manager::ui_menu() {

	using namespace ImGui;

	auto mode_button = [this](Gui::Mode m, std::string name) -> bool {
		bool active = m == mode;
		if (active) PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
		bool clicked = Button(name.c_str());
		if (active) PopStyleColor();
		return clicked;
	};

	float menu_height = 0.0f;
	if (BeginMainMenuBar()) {

		if (BeginMenu("File")) {

			if (MenuItem("New Scene")) new_scene();
			if (MenuItem("Open Scene (Ctrl+o)")) load_scene(nullptr, Load::new_scene);
			if (MenuItem("Save Scene As (Ctrl+e)")) save_scene_as();
			if (MenuItem("Save Scene (Ctrl+s)")) save_scene_as((save_file.empty() ? nullptr : &save_file));
			ImGui::EndMenu();
		}

		if (BeginMenu("Edit")) {

			if (MenuItem("Undo (Ctrl+z)")) undo.undo();
			if (MenuItem("Redo (Ctrl+y)")) undo.redo();
			if (MenuItem("Edit Debug Data (Ctrl+d)")) debug_shown = true;
			if (MenuItem("Settings")) settings_shown = true;
			ImGui::EndMenu();
		}

		if (mode_button(Gui::Mode::layout, "Layout")) {
			mode = Gui::Mode::layout;
			if (widgets.active == Widget_Type::bevel) widgets.active = Widget_Type::move;
		}

		if (mode_button(Gui::Mode::model, "Model")) mode = Gui::Mode::model;

		if (mode_button(Gui::Mode::render, "Render")) mode = Gui::Mode::render;

		if (mode_button(Gui::Mode::rig, "Rig")) mode = Gui::Mode::rig;

		if (mode_button(Gui::Mode::animate, "Animate")) mode = Gui::Mode::animate;

		if (mode_button(Gui::Mode::simulate, "Simulate")) mode = Gui::Mode::simulate;

		Text("FPS: %.0f", GetIO().Framerate);

		menu_height = GetWindowSize().y;
		EndMainMenuBar();
	}

	return menu_height;
}

void Manager::end_drag() {

	if (!widgets.is_dragging()) return;

	if (!selected_instance_transform.expired()) {
		transform_widget.update(undo, selected_instance_transform);
	}

	switch (mode) {
	case Mode::layout:
	case Mode::simulate:
	case Mode::render: break;
	case Mode::model: {
		model.end_transform(undo);
	} break;
	case Mode::rig: {
		rig.end_transform(widgets, undo);
	} break;
	case Mode::animate: {
		animate.end_transform(undo);
	} break;
	default: assert(false);
	}

	widgets.end_drag();
}

void Manager::drag_to(Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods) {

	if (!widgets.is_dragging()) return;

	//for controls that need to know about world<->local xform:
	Mat4 local_to_world;
	if (auto transform = selected_instance_transform.lock()) {
		local_to_world = transform->local_to_world();
	} else {
		local_to_world = Mat4::I;
	}

	std::optional<Vec3> pos;
	float snap = 0.0f;

	if (mods & SnapBit) {
		if (widgets.active == Widget_Type::rotate) {
			snap = 15.0f;
		} else if (widgets.active == Widget_Type::move) {
			snap = 1.0f;
		}
	}

	switch (mode) {
	case Mode::layout:
	case Mode::simulate:
	case Mode::render: {
		if (!selected_instance_transform.expired()) {
			auto t = selected_instance_transform.lock();
			pos = t->local_to_world() * Vec3{};
		}
	} break;
	case Mode::model: {
		pos = model.selected_pos();
	} break;
	case Mode::rig: {
		pos = rig.selected_pos();
	} break;
	case Mode::animate: {
		pos = animate.selected_pos(local_to_world);
	} break;
	default: assert(false);
	}

	if (pos.has_value()) {
		widgets.drag_to(pos.value(), cam, spos, dir, mode == Mode::model, snap);
	}

	switch (mode) {
	case Mode::layout:
	case Mode::simulate:
	case Mode::render: {
		if (!selected_instance_transform.expired()) {
			auto t = selected_instance_transform.lock();
			*t = widgets.apply_action(transform_widget.cache);
		}
	} break;
	case Mode::model: {
		model.apply_transform(widgets);
	} break;
	case Mode::rig: {
		rig.apply_transform(widgets);
	} break;
	case Mode::animate: {
		if (!animate.apply_transform(widgets, local_to_world)) {
			if (!selected_instance_transform.expired()) {
				auto t = selected_instance_transform.lock();
				*t = widgets.apply_action(transform_widget.cache);
			}
		}
	} break;
	default: assert(false);
	}
}

void Manager::set_select(uint32_t id) {
	auto entry = id_to_instance.find(id);
	if (entry != id_to_instance.end()) {
		set_select(entry->second);
	} else {
		clear_select();
	}
}

void Manager::set_select(const std::string& name) {
	clear_select();
	selected_object_name = name;
	bool updated_animate = false;
	scene.for_each_instance([&](const std::string& i_name, auto&& resource) {
		if (name == i_name) {
			selected_instance_transform = resource->transform;
			if constexpr (std::is_same_v<std::decay_t<decltype(*resource)>,
			                             Instance::Skinned_Mesh>) {
				if (!resource->mesh.expired()) {
					animate.set_mesh(scene.name<Skinned_Mesh>(resource->mesh).value(),
					                 resource->mesh);
					updated_animate = true;
				}
			}
		}
	});
	if (!updated_animate) animate.set_mesh({}, {});
}

void Manager::clear_select() {
	switch (mode) {
	case Mode::animate: {
		selected_object_name = std::nullopt;
		selected_instance_transform = std::weak_ptr<Transform>{};
		animate.clear_select();
	} break;
	case Mode::layout:
	case Mode::simulate:
	case Mode::render: {
		selected_object_name = std::nullopt;
		selected_instance_transform = std::weak_ptr<Transform>{};
	} break;
	case Mode::model: {
		model.clear_select();
	} break;
	case Mode::rig: {
		rig.clear_select();
	} break;
	default: assert(false);
	}
}

bool Manager::select(uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods) {

	bool dragging = widgets.select(id);

	switch (mode) {
	case Mode::animate: {
		Mat4 p = selected_instance_transform.expired()
		             ? Mat4::I
		             : selected_instance_transform.lock()->local_to_world();
		bool drag_started = animate.select(scene, widgets, p, id, cam, spos, dir);
		if (drag_started) {
			assert(!selected_instance_transform.expired());
			transform_widget.cache = *selected_instance_transform.lock();
		}
	} break;
	case Mode::layout:
	case Mode::simulate:
	case Mode::render: {
		if (dragging) {
			assert(!selected_instance_transform.expired());
			transform_widget.cache = *selected_instance_transform.lock();
			widgets.start_drag(transform_widget.cache.local_to_world() * Vec3{}, cam, spos, dir);
		} else {
			set_select(id);
		}
	} break;
	case Mode::model: {
		set_error(model.select(widgets, id, cam, spos, dir, mods));
	} break;
	case Mode::rig: {
		rig.select(scene, widgets, undo, id, cam, spos, dir);
	} break;
	default: assert(false);
	}

	return widgets.is_dragging();
}

void Manager::render_3d(View_3D& gui_cam) {

	Mat4 view = gui_cam.get_view();

	Renderer::get().lines(baseplane, view, Mat4::I, 1.0f);

	update_gpu_caches();

	animate.update(scene, animator);

	if (mode != Mode::model && mode != Mode::rig) {
		if (mode != Mode::animate && !animate.playing_or_rendering()) {
			simulate.resume();
			simulate.update(scene, undo);
		} else {
			simulate.pause();
		}
	}

	auto do_widgets = [&]() {
		if (!selected_instance_transform.expired()) {
			auto T = selected_instance_transform.lock()->local_to_world();
			Vec3 pos = T * Vec3{};
			float scale = std::min((gui_cam.pos() - pos).norm() / 5.5f, 10.0f);
			widgets.render(view, pos, scale);
		}
	};

	switch (mode) {
	case Mode::render: {
		selected_instance_transform = render_instances(view);
		render.render(gui_cam);
		do_widgets();
	} break;
	case Mode::simulate: {
	case Mode::layout: {
		selected_instance_transform = render_instances(view);
		do_widgets();
	} break;
	case Mode::model: {
		model.render(widgets, gui_cam);
	} break;
	case Mode::rig: {
		rig.render(widgets, gui_cam);
	} break;
	case Mode::animate: {
		selected_instance_transform = render_instances(view);
		Mat4 p = selected_instance_transform.expired()
		             ? Mat4::I
		             : selected_instance_transform.lock()->local_to_world();
		bool did_widgets = animate.render(scene, widgets, p, next_id, gui_cam);
		if (!did_widgets) do_widgets();
	} break;
	default: assert(false);
	}
	}
}

std::weak_ptr<Transform> Manager::render_instances(Mat4 view, bool gui) {

	update_gpu_caches();

	std::weak_ptr<Transform> ret;
	id_to_instance.clear();

	std::unordered_map<std::shared_ptr<Halfedge_Mesh>, std::string> mesh_names;
	std::unordered_map<std::shared_ptr<Skinned_Mesh>, std::string> skinned_mesh_names;
	std::unordered_map<std::shared_ptr<Shape>, std::string> shape_names;
	std::unordered_map<std::shared_ptr<Texture>, std::string> texture_names;
	std::unordered_map<std::shared_ptr<Delta_Light>, std::string> delta_light_names;
	std::unordered_map<std::shared_ptr<Camera>, std::string> camera_names;
	{
		for (const auto& [name, mesh] : scene.meshes) {
			mesh_names[mesh] = name;
		}
		for (const auto& [name, mesh] : scene.skinned_meshes) {
			skinned_mesh_names[mesh] = name;
		}
		for (const auto& [name, shape] : scene.shapes) {
			shape_names[shape] = name;
		}
		for (const auto& [name, texture] : scene.textures) {
			texture_names[texture] = name;
		}
		for (const auto& [name, light] : scene.delta_lights) {
			delta_light_names[light] = name;
		}
		for (const auto& [name, camera] : scene.cameras) {
			camera_names[camera] = name;
		}
	}

	next_id = static_cast<uint32_t>(Widget_IDs::count);

	auto make_meshopt = [&](const auto& inst, bool world_space = false) {
		Mat4 model = (world_space || inst->transform.expired()) ? Mat4::I : inst->transform.lock()->local_to_world();

		Renderer::MeshOpt opt;
		opt.id = next_id;
		opt.modelview = view * model;
		if constexpr (std::is_same< decltype(inst->settings),Instance::Geometry_Settings >::value) {
			opt.wireframe = inst->settings.draw_style == DrawStyle::Wireframe;
		} else {
			opt.wireframe = inst->settings.wireframe;
		}
		if (inst->material.expired()) {
			opt.color = Color::black;
			opt.solid_color = true;
		} else {
			opt.solid_color = inst->material.lock()->is_emissive();
			auto texture = inst->material.lock()->display();
			if (texture.expired()) {
				opt.color = Color::black;
			} else if (std::holds_alternative<Textures::Image>(texture.lock()->texture)) {
				gpu_texture_cache.at(texture_names.at(texture.lock())).bind();
				opt.use_texture = true;
			} else if (std::holds_alternative<Textures::Constant>(texture.lock()->texture)) {
				opt.color = std::get<Textures::Constant>(texture.lock()->texture).color;
			} else {
				die("Can't render this texture type!");
			}
		}

		return opt;
	};

	auto render_mesh = [&](const std::string& obj_name, GL::Mesh& mesh,
	                       const Renderer::MeshOpt& opt) {
		if (obj_name == selected_object_name && gui &&
		    !(mode == Mode::animate && animate.skel_selected())) {
			Renderer::get().begin_outline();
			Renderer::get().mesh(mesh, opt);
			Renderer::get().end_outline(mesh.bbox().transform(opt.modelview));
		}
		Renderer::get().mesh(mesh, opt);
	};

	// TODO: It would be nice to gather all instances of the cached GL::Mesh and render them
	// with one instanced draw call, but we don't currently support changing all the render
	// settings for each instance.
	for (const auto& [name, inst] : scene.instances.meshes) {
		if (name == selected_object_name) ret = inst->transform;
		if (!inst->settings.visible) continue;
		if (inst->mesh.expired()) continue;

		auto opt = make_meshopt(inst);
		auto mesh_name = mesh_names.at(inst->mesh.lock());
		render_mesh(name, gpu_mesh_cache.at(mesh_name), opt);

		id_to_instance[next_id++] = name;
	}
	for (const auto& [name, inst] : scene.instances.skinned_meshes) {
		if (name == selected_object_name) ret = inst->transform;
		if (!inst->settings.visible) continue;
		if (inst->mesh.expired()) continue;

		auto opt = make_meshopt(inst);
		auto mesh_name = skinned_mesh_names.at(inst->mesh.lock());
		render_mesh(name, gpu_mesh_cache.at(mesh_name), opt);

		id_to_instance[next_id++] = name;
	}
	for (const auto& [name, inst] : scene.instances.shapes) {
		if (name == selected_object_name) ret = inst->transform;
		if (!inst->settings.visible) continue;
		if (inst->shape.expired()) continue;

		auto opt = make_meshopt(inst);
		auto mesh_name = shape_names.at(inst->shape.lock());
		render_mesh(name, gpu_mesh_cache.at(mesh_name), opt);

		id_to_instance[next_id++] = name;
	}
	for (const auto& [name, inst] : scene.instances.delta_lights) {
		if (name == selected_object_name) ret = inst->transform;
		if (!inst->settings.visible) continue;
		if (inst->light.expired()) continue;
		if (!gui) continue;

		auto light = inst->light.lock();
		Mat4 m = inst->transform.expired() ? Mat4::I : inst->transform.lock()->local_to_world();

		Renderer::MeshOpt opt;
		opt.id = next_id;
		opt.modelview = view * m;
		opt.solid_color = true;
		opt.color = light->display();

		if (light->is<Delta_Lights::Spot>()) {
			auto lines_name = delta_light_names[inst->light.lock()];
			Renderer::get().lines(gpu_lines_cache.at(lines_name), view, m);
			render_mesh(name, spot_light_origin_mesh, opt);
		} else if (light->is<Delta_Lights::Point>()) {
			render_mesh(name, point_light_mesh, opt);
		} else if (light->is<Delta_Lights::Directional>()) {
			render_mesh(name, directional_light_mesh, opt);
		} else {
			die("Can't render this light type!");
		}

		id_to_instance[next_id++] = name;
	}
	for (const auto& [name, inst] : scene.instances.env_lights) {
		if (!inst->settings.visible) continue;
		if (inst->light.expired()) continue;

		auto light = inst->light.lock();
		float cosine = -1.0f;
		if (light->is<Environment_Lights::Hemisphere>()) {
			cosine = 0.0f;
		}

		Mat4 rot = view;
		rot.cols[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

		auto texture = light->display();
		if (texture.expired()) {
			Renderer::get().skydome(rot, Color::black, cosine);
		} else if (texture.lock()->is<Textures::Constant>()) {
			Spectrum color = std::get<Textures::Constant>(texture.lock()->texture).color;
			Renderer::get().skydome(rot, color, cosine);
		} else if (texture.lock()->is<Textures::Image>()) {
			Renderer::get().skydome(rot, Color::black, cosine,
			                        gpu_texture_cache.at(texture_names.at(texture.lock())));
		} else {
			die("Can't render this texture type!");
		}

		id_to_instance[next_id++] = name;
	}

	std::unordered_map<std::shared_ptr<Particles>, GL::Instances> particle_instance_cache;
	for (const auto& [_, particles] : scene.particles) {
		GL::Instances instances;
		instances.clear(particles->particles.size());
		for (auto& p : particles->particles) {
			float s = particles->radius;
			Vec3 pos = p.position;
			instances.add(Mat4{Vec4{s, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, s, 0.0f, 0.0f},
			                   Vec4{0.0f, 0.0f, s, 0.0f}, Vec4{pos.x, pos.y, pos.z, 1.0f}},
			              next_id);
		}
		particle_instance_cache[particles] = std::move(instances);
	}
	for (const auto& [name, inst] : scene.instances.particles) {
		if (name == selected_object_name) ret = inst->transform;
		if (!inst->settings.visible) continue;


		if (!inst->mesh.expired() && !inst->particles.expired()) {
			Renderer::MeshOpt opt = make_meshopt(inst, true); //no model transform (particles are simulated in world space)
			auto mesh_name = mesh_names[inst->mesh.lock()];
			auto particles = inst->particles.lock();
			Renderer::get().instances(particle_instance_cache.at(particles), gpu_mesh_cache.at(mesh_name), opt);
		}

		Renderer::MeshOpt opt = make_meshopt(inst);
		opt.solid_color = true;
		opt.wireframe = false;
		render_mesh(name, particle_system_mesh, opt);

		id_to_instance[next_id++] = name;
	}

	for (const auto& [name, inst] : scene.instances.cameras) {
		if (name == selected_object_name) ret = inst->transform;
		if (inst->camera.expired()) continue;
		if (!gui) continue;

		auto camera = inst->camera.lock();
		Mat4 m = inst->transform.expired() ? Mat4::I : inst->transform.lock()->local_to_world();

		auto lines_name = camera_names[inst->camera.lock()];
		Renderer::get().lines(gpu_lines_cache.at(lines_name), view, m);

		id_to_instance[next_id++] = name;
	}

	for (const auto& [name, transform] : scene.transforms) {
		if (name == selected_object_name) ret = transform;
	}

	return ret;
}

void Manager::hover(uint32_t id, Vec3 cam, Vec2 spos, Vec3 dir, Modifiers mods) {
	if (mode == Mode::model) {
		model.hover(id);
	} else if (mode == Mode::rig) {
		rig.hover(cam, spos, dir);
	}
}

void Manager::update_gpu_caches() {
	for (const auto& [name, mesh] : scene.meshes) {
		if (gpu_mesh_cache.find(name) == gpu_mesh_cache.end()) {
			gpu_mesh_cache[name] =
				Indexed_Mesh::from_halfedge_mesh(*mesh, Indexed_Mesh::SplitEdges).to_gl();
		}
	}
	for (const auto& [name, mesh] : scene.skinned_meshes) {
		if (gpu_mesh_cache.find(name) == gpu_mesh_cache.end()) {
			gpu_mesh_cache[name] = mesh->posed_mesh().to_gl();
		}
	}
	for (const auto& [name, shape] : scene.shapes) {
		if (gpu_mesh_cache.find(name) == gpu_mesh_cache.end()) {
			gpu_mesh_cache[name] = shape->to_mesh().to_gl();
		}
	}
	for (const auto& [name, texture] : scene.textures) {
		if (texture->is<Textures::Image>()) {
			if (gpu_texture_cache.find(name) == gpu_texture_cache.end()) {
				gpu_texture_cache[name] = std::get<Textures::Image>(texture->texture).to_gl();
			}
		}
	}
	for (const auto& [name, light] : scene.delta_lights) {
		if (light->is<Delta_Lights::Spot>()) {
			if (gpu_lines_cache.find(name) == gpu_lines_cache.end()) {
				gpu_lines_cache[name] = std::get<Delta_Lights::Spot>(light->light).to_gl();
			}
		}
	}
	for (const auto& [name, camera] : scene.cameras) {
		if (gpu_lines_cache.find(name) == gpu_lines_cache.end()) {
			gpu_lines_cache[name] = camera->to_gl();
		}
	}
}

void Manager::invalidate_gpu(const std::string& name) {
	gpu_mesh_cache.erase(name);
	gpu_lines_cache.erase(name);
	gpu_texture_cache.erase(name);
	model.invalidate(name);
	rig.invalidate(name);
	animate.invalidate(name);
}

template bool Manager::choose_instance<Material, Texture>(const std::string&, const std::shared_ptr<Material>&, std::weak_ptr<Texture>&, bool);
template bool Manager::choose_instance<Environment_Light, Texture>(const std::string&, const std::shared_ptr<Environment_Light>&, std::weak_ptr<Texture>&, bool);
template bool Manager::choose_instance<Instance::Camera, Transform>(const std::string&, const std::shared_ptr<Instance::Camera>&, std::weak_ptr<Transform>&, bool);

} // namespace Gui
