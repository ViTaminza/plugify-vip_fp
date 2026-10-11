// vip_fp unit tests — the engine-free part: the C4 weapon class name test of the original
// (a case-sensitive strstr for "c4").
//
// Run by ../../test.sh (inside the Steam Runtime image, cwd = build/tests).

#include "fp_logic.hpp"

#include <cstdio>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void CheckEq(bool actual, bool expected, const std::string& what) {
	++g_checks;
	if (actual != expected) {
		++g_failures;
		std::printf("FAIL: %s (expected %d, got %d)\n", what.c_str(), expected ? 1 : 0, actual ? 1 : 0);
	}
}

} // namespace

int main() {
	std::printf("vip_fp tests\n");

	CheckEq(vip_fp::IsC4Classname("weapon_c4"), true, "the held C4 weapon matches");
	CheckEq(vip_fp::IsC4Classname("weapon_ak47"), false, "an ordinary weapon does not");
	CheckEq(vip_fp::IsC4Classname(""), false, "an empty class name does not");
	CheckEq(vip_fp::IsC4Classname("CC4"), false, "the schema class name is case sensitive (like strstr)");
	CheckEq(vip_fp::IsC4Classname("weapon_c4_planted"), true, "a longer name containing c4 matches");
	CheckEq(vip_fp::IsC4Classname("de_c4dummy"), true, "any c4 substring matches, the original's test");

	// A handle with bit 31 set: index 93, serial 102949 -> 0xC912805D = -921534371, a *valid* weapon
	// handle handed out once the engine's serial counter has passed 65535.
	CheckEq(vip_fp::IsValidHandle(-921534371), true, "a negative handle (serial > 65535) is valid");
	CheckEq(vip_fp::IsValidHandle(0x7FFFFFFF), true, "a positive handle is valid");
	CheckEq(vip_fp::IsValidHandle(1), true, "handle 1 is valid");
	CheckEq(vip_fp::IsValidHandle(0), false, "0 is not a handle (no active weapon)");
	CheckEq(vip_fp::IsValidHandle(-1), false, "-1 (INVALID_EHANDLE_INDEX) is not a handle");

	std::printf("%d checks, %d failures\n", g_checks, g_failures);
	return g_failures == 0 ? 0 : 1;
}
