
#pragma once

#include "imgui.h"

namespace ImGui {

template<typename E>
bool Combo(const char* label, E* current_item, const char* const items[(int)E::count],
           int items_count = (int)E::count, int popup_max_height_in_items = -1) {
	int v = static_cast<int>(*current_item);
	bool ret = Combo(label, &v, items, static_cast<int>(E::count), popup_max_height_in_items);
	*current_item = static_cast<E>(v);
	return ret;
}

inline bool InputUInt32(const char* label, uint32_t* v_, int step = 1, int step_fast = 100,
                        ImGuiInputTextFlags flags = 0) {
	int v = static_cast<int>(*v_);
	bool ret = InputInt(label, &v, step, step_fast, flags);
	if (v < 0)
		*v_ = 0;
	else
		*v_ = v;
	return ret;
}

inline bool SliderUInt32(const char* label, uint32_t* v_, uint32_t v_min, uint32_t v_max,
                         const char* format = "%d") {
	int v = static_cast<int>(*v_);
	bool ret = SliderInt(label, &v, static_cast<int>(v_min), static_cast<int>(v_max), format);
	*v_ = v;
	return ret;
}

inline bool WrapButton(std::string label) {
	ImGuiStyle& style = GetStyle();
	float available_w = GetWindowPos().x + GetWindowContentRegionMax().x;
	float last_w = GetItemRectMax().x;
	float next_w =
		last_w + style.ItemSpacing.x + CalcTextSize(label.c_str()).x + style.FramePadding.x * 2;
	if (next_w < available_w) SameLine();
	return Button(label.c_str());
}

} // namespace ImGui
