
#include "undo.h"

Undo::Undo(Scene& sc, Animator& an, Gui::Manager& manager)
	: scene(sc), animator(an), manager(manager) {
}

void Undo::reset() {
	undos = {};
	redos = {};
}

void Undo::action(std::unique_ptr<Action_Base>&& action) {
	redos = {};
	undos.push(std::move(action));
	total_actions++;
}

void Undo::undo() {
	if (undos.empty()) return;
	undos.top()->undo();
	redos.push(std::move(undos.top()));
	undos.pop();
	total_actions++;
}

void Undo::redo() {
	if (redos.empty()) return;
	redos.top()->redo();
	undos.push(std::move(redos.top()));
	redos.pop();
	total_actions++;
}

void Undo::bundle_last(size_t n) {
	if (!n) return;
	std::vector<std::unique_ptr<Action_Base>> undo_pack;
	for (size_t i = 0; i < n; i++) {
		undo_pack.push_back(std::move(undos.top()));
		undos.pop();
	}
	undos.push(std::make_unique<Action_Bundle>(std::move(undo_pack)));
}

size_t Undo::n_actions() {
	return total_actions;
}

void Undo::inc_actions() {
	total_actions++;
}

void Undo::anim_set_max_frame(Gui::Animate& animate, uint32_t new_max_frame,
                              uint32_t old_max_frame) {

	Animator old_animator = animator;

	animate.set_max(new_max_frame);
	animator.crop(static_cast<float>(new_max_frame));

	Animator new_animator = animator;

	action(
		[&, new_max_frame, new_animator = std::move(new_animator)]() {
			animate.set_max(new_max_frame);
			animator = new_animator;
		},
		[&, old_max_frame, old_animator = std::move(old_animator)]() {
			animate.set_max(old_max_frame);
			animator = old_animator;
		});
}

void Undo::anim_set_keyframe(const std::string& name, float key) {
	
	Animator old_animator = animator;
	animator.set_all(scene, name, key);
	Animator new_animator = animator;

	action(
		[&, new_animator = std::move(new_animator)]() {
			animator = new_animator;
		},
		[&, old_animator = std::move(old_animator)]() {
			animator = old_animator;
		});
}

void Undo::anim_clear_keyframe(const std::string& name, float key) {

	Animator old_animator = animator;
	animator.erase_all(scene, name, key);
	Animator new_animator = animator;

	action(
		[&, new_animator = std::move(new_animator)]() {
			animator = new_animator;
		},
		[&, old_animator = std::move(old_animator)]() {
			animator = old_animator;
		});
}
