// vip_fp — engine-free part of the Fast Plant module: the original's weapon class name test
// (vip_fp.cpp).
//
// Kept pure so it is unit-testable (tests/vip_fp_tests.cpp).

#pragma once

#include <cstdint>
#include <string_view>

namespace vip_fp {

// Entity HANDLES are int32 views of the engine's uint32 CEntityHandle (index with the serial in
// the upper bits). A handle is valid when it is neither INVALID_EHANDLE_INDEX (-1) nor 0 (index 0
// is the world; GetClientActiveWeapon answers 0/-1 when there is no active weapon). The sign means
// nothing - once the serial counter passes 65535 bit 31 is set, and a `handle > 0` test silently
// drops those weapons.
inline bool IsValidHandle(int32_t h) { return h != -1 && h != 0; }

// The original's class-name test: a case-sensitive substring search for "c4", so "weapon_c4"
// matches and the schema class "CC4" does not (in game the weapon's class name is "weapon_c4").
inline bool IsC4Classname(std::string_view classname) {
	return classname.find("c4") != std::string_view::npos;
}

} // namespace vip_fp
