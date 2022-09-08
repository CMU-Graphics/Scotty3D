
#pragma once

#include <memory>
#include <stack>

#include "../gui/manager.h"
#include "../gui/widgets.h"
#include "../util/viewer.h"

#include "animator.h"
#include "scene.h"

template<typename T> class Action_Update_Cached;

class Action_Base {
	virtual void undo() = 0;
	virtual void redo() = 0;
	friend class Undo;
	friend class Action_Bundle;

public:
	virtual ~Action_Base() = default;
};

template<typename R, typename U> class Action_Fn : public Action_Base {
	void undo() {
		_undo();
	}
	void redo() {
		_redo();
	}
	R _redo;
	U _undo;

public:
	Action_Fn(R&& _redo, U&& _undo) : _redo(std::move(_redo)), _undo(std::move(_undo)) {}
	~Action_Fn() = default;
};

class Action_Bundle : public Action_Base {
	void undo() {
		for (auto& a : list) a->undo();
	}
	void redo() {
		for (auto i = list.rbegin(); i != list.rend(); i++) (*i)->redo();
	}

	std::vector<std::unique_ptr<Action_Base>> list;

public:
	Action_Bundle(std::vector<std::unique_ptr<Action_Base>>&& bundle) : list(std::move(bundle)){};
	~Action_Bundle() = default;
};

template<typename T> class Action_Create : public Action_Base {
	void undo() {
		resource = scene.remove<T>(name);
	}
	void redo() {
		name = scene.insert<T>(name, std::move(resource));
	}

	std::string name;
	std::shared_ptr<T> resource;
	Scene& scene;

public:
	Action_Create(const std::string& name, Scene& scene) : name(name), scene(scene){};
	~Action_Create() = default;
};

template<typename T> class Action_Erase : public Action_Base {
	void undo() {
		name = scene.insert<T>(name, std::move(resource));
	}
	void redo() {
		resource = scene.remove<T>(name);
	}

	std::string name;
	std::shared_ptr<T> resource;
	Scene& scene;

public:
	Action_Erase(const std::string& name, std::shared_ptr<T>&& resource, Scene& scene)
		: name(name), resource(std::move(resource)), scene(scene){};
	~Action_Erase() = default;
};

template<typename T> class Action_Update : public Action_Base {
	virtual void redo() {
		if (auto r = resource.lock()) {
			old_value = std::move(*r);
			*r = std::move(new_value);
		}
	}
	virtual void undo() {
		if (auto r = resource.lock()) {
			new_value = std::move(*r);
			*r = std::move(old_value);
		}
	}

	std::weak_ptr<T> resource;
	T old_value, new_value;

public:
	Action_Update(std::weak_ptr<T> resource, T old_value)
		: resource(resource), old_value(std::move(old_value)) {
	}
	virtual ~Action_Update() = default;

	friend class Action_Update_Cached<T>;
};

template<typename T> class Action_Update_Cached : public Action_Update<T> {
	void redo() {
		Action_Update<T>::redo();
		manager.invalidate_gpu(name);
	}
	void undo() {
		Action_Update<T>::undo();
		manager.invalidate_gpu(name);
	}

	std::string name;
	Gui::Manager& manager;

public:
	Action_Update_Cached(Gui::Manager& manager, const std::string& name, std::weak_ptr<T> resource,
	                     T old_value)
		: Action_Update<T>(std::move(resource), std::move(old_value)), name(name),
		  manager(manager){};
	~Action_Update_Cached() = default;
};

class Action_Rename : public Action_Base {
	void redo() {
		new_name = scene.rename(old_name, new_name).value_or("");
		animator.rename(old_name, new_name);
	}
	void undo() {
		old_name = scene.rename(new_name, old_name).value_or("");
		animator.rename(new_name, old_name);
	}

	std::string old_name, new_name;
	Scene& scene;
	Animator& animator;

public:
	Action_Rename(const std::string& old_name, const std::string& new_name, Scene& scene,
	              Animator& animator)
		: old_name(old_name), new_name(new_name), scene(scene), animator(animator){};
	~Action_Rename() = default;
};

class Undo {
public:
	Undo(Scene& scene, Animator& animator, Gui::Manager& manager);

	template<typename T> std::string create(const std::string& name, T&& obj) {
		auto name_unique = scene.create(name, std::forward<T&&>(obj));
		action(std::make_unique<Action_Create<T>>(name_unique, scene));
		return name_unique;
	}

	template<typename T> void erase(const std::string& name) {
		auto resource = scene.remove<T>(name);
		if (!resource) return;
		action(std::make_unique<Action_Erase<T>>(name, std::move(resource), scene));
	}

	template<typename T> void update(std::weak_ptr<T> resource, T old_value) {

		static_assert(!std::is_same_v<T, Halfedge_Mesh>, "Did you want update_cached?");
		static_assert(!std::is_same_v<T, Skinned_Mesh>, "Did you want update_cached?");
		static_assert(!std::is_same_v<T, Shape>, "Did you want update_cached?");
		static_assert(!std::is_same_v<T, Texture>, "Did you want update_cached?");
		static_assert(!std::is_same_v<T, Delta_Light>, "Did you want update_cached?");
		static_assert(!std::is_same_v<T, Camera>, "Did you want update_cached?");

		if (resource.expired()) return;
		action(std::make_unique<Action_Update<T>>(resource, std::move(old_value)));
	}

	template<typename T>
	void update_cached(const std::string& name, std::weak_ptr<T> resource, T old_value) {
		if (resource.expired()) return;
		manager.invalidate_gpu(name);
		action(std::make_unique<Action_Update_Cached<T>>(manager, name, resource,
		                                                 std::move(old_value)));
	}

	void rename(const std::string& old_name, const std::string& new_name) {
		scene.rename(old_name, new_name);
		action(std::make_unique<Action_Rename>(old_name, new_name, scene, animator));
	}

	void invalidate(const std::string& name) {
		manager.invalidate_gpu(name);
	}

	void undo();
	void redo();
	void reset();
	size_t n_actions();
	void inc_actions();
	void bundle_last(size_t n);

	void anim_set_max_frame(Gui::Animate& animate, uint32_t new_max_frame, uint32_t old_max_frame);
	void anim_set_keyframe(const std::string& name, float key);
	void anim_clear_keyframe(const std::string& name, float key);

private:
	Scene& scene;
	Animator& animator;
	Gui::Manager& manager;

	void action(std::unique_ptr<Action_Base>&& action);
	template<typename R, typename U> void action(R&& redo, U&& undo) {
		action(std::make_unique<Action_Fn<R, U>>(std::move(redo), std::move(undo)));
	}

	std::stack<std::unique_ptr<Action_Base>> undos;
	std::stack<std::unique_ptr<Action_Base>> redos;
	size_t total_actions = 0;
};