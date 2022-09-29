#pragma once

#include <cstdint>

namespace Gui {
	// Modifier keys modulate mouse actions:
	using Modifiers = uint8_t;
	enum : Modifiers {
		SnapBit = 0x01, //snap-to-grid (ctrl)
		AppendBit = 0x02, //append/toggle selection (shift)
	};
}
