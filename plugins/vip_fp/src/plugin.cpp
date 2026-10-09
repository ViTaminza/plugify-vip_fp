// vip_fp — Plugify port of the Pisex VIP module "VIP_FastPlant" / "[VIP] Fast Plant"
// (ref/src/cs2-vip-modules/VIP_FastPlant/src/vip_fp.cpp, 133 lines).
//
// Behaviour kept identical:
//   * feature "fp" (VIP_BOOL, TOGGLABLE).
//   * a "bomb_beginplant" pre-hook: when the planting player is a VIP with the feature, has a
//     living pawn and holds an active weapon whose class name contains "c4", the weapon's
//     m_fArmedTime is set to the current game time - i.e. the plant starts "already armed", which
//     is what makes it fast (the same field write the original performed).
//   * the original's early-outs: not a VIP / feature off, no pawn or dead pawn, no weapon
//     services, no active weapon, class name without "c4".
//
// Port notes:
//   * pPlayerPawn->m_pWeaponServices()->m_hActiveWeapon() -> s2sdk::GetClientActiveWeapon (the
//     same field, returned as the raw entity handle) and s2sdk::GetEntityClassname.
//   * m_fArmedTime lives on the CC4 schema class of the held C4 entity ("weapon_c4"); it is
//     written through the schema on the raw entity pointer. The original's
//     `m_fArmedTime().SetTime(curtime)` wrote the wrapped GameTime_t value without a network
//     state change, so the port writes with changeState = false as well.
//   * gpGlobals->curtime -> pisex::GetCGlobalVars()->curtime (the pisexcompat snapshot, at most
//     one frame old - the same value the game clock had when the engine fired the event).

#include <pisex/pisex.hpp>
#include <plg/plugin.hpp>
#include <plugin_export.h>
#include <s2sdk/s2sdk.hpp>
#include <vip_api.hpp>

#include "fp_logic.hpp"

#include <chrono>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>

namespace {

// The original code's unqualified spellings (VIP_BOOL, TOGGLABLE, ...).
using namespace vip;

constexpr int kMaxSlots = 64;
constexpr const char* kLogTag = "VIP-FP"; // GetLogTag() of the original
constexpr const char* kFeature = "fp";

IVIPApi* g_pVIPCore = &IVIPApi::Get();

void LogInfo(std::string_view message) {
	if (plg::Log) {
		plg::Log(message, plg::Severity::Info, plg::source_location::current());
	}
}

void LogError(std::string_view message) {
	if (plg::Log) {
		plg::Log(message, plg::Severity::Error, plg::source_location::current());
	}
}

// The original's ErrorLog("[%s] ...", GetLogTag()).
void LogTaggedError(const std::string& message) {
	LogError(std::string("[") + kLogTag + "] " + message);
}

// pPlayerPawn->m_pWeaponServices() (only the original's null check), read from the raw pawn
// pointer; the component pointer has no s2sdk helper.
void* WeaponServices(int iSlot) {
	// s2sdk's GetClientPawn warns "invalid player slot: N" (to the server console) for every call
	// on a slot without a controller, so an empty slot is filtered out here.
	if (!__s2sdk_GetClientPawn || iSlot < 0 || iSlot >= kMaxSlots || !pisex::IsConnected(iSlot)) {
		return nullptr;
	}
	try {
		void* pawn = s2sdk::GetClientPawn(iSlot);
		if (!pawn) {
			return nullptr;
		}
		const int offset = pisex::SchemaOffset("CBasePlayerPawn", "m_pWeaponServices");
		if (offset < 0) {
			return nullptr;
		}
		// m_pWeaponServices is a pointer field: its value is the component address.
		return *reinterpret_cast<void**>(static_cast<char*>(pawn) + offset);
	} catch (...) {
		return nullptr;
	}
}

class FastPlantPlugin final : public plg::Plugin {
public:
	plg::PluginResult OnPluginStart() override;
	plg::PluginResult OnPluginUpdate(std::chrono::milliseconds dt) override;
	plg::PluginResult OnPluginEnd() override;

private:
	// OnBeginPlant of the original (the "bomb_beginplant" hook).
	void OnBeginPlant(const char* szName, pisex::IGameEvent* pEvent, bool bDontBroadcast);
};

FastPlantPlugin g_plugin;

void FastPlantPlugin::OnBeginPlant(const char* /*szName*/, pisex::IGameEvent* pEvent, bool /*bDontBroadcast*/) {
	if (!pEvent || !pEvent->IsValid()) {
		return;
	}
	// int iSlot = pEvent->GetInt("userid"); bool bValue = VIP_GetClientFeatureBool(iSlot, "fp");
	const int iSlot = pEvent->GetPlayerSlot("userid");
	if (iSlot < 0 || iSlot >= kMaxSlots || !__s2sdk_GetClientActiveWeapon || !__s2sdk_GetEntityClassname ||
		!__s2sdk_EntHandleToEntIndex || !__s2sdk_IsValidEntHandle || !__s2sdk_SetEntSchemaFloat) {
		return;
	}
	const bool bValue = g_pVIPCore->VIP_GetClientFeatureBool(iSlot, kFeature);
	if (!g_pVIPCore->VIP_IsClientVIP(iSlot) || !bValue) {
		return;
	}
	// CCSPlayerController + m_hPlayerPawn + IsAlive.
	if (!pisex::IsAlive(iSlot)) {
		return;
	}
	// CPlayer_WeaponServices* pWeaponServices = ...; if(!pWeaponServices) return;
	if (WeaponServices(iSlot) == nullptr) {
		return;
	}
	try {
		// CEntityInstance* hActiveWeapon = (CEntityInstance*)pWeaponServices->m_hActiveWeapon();
		const int handle = s2sdk::GetClientActiveWeapon(iSlot);
		if (!vip_fp::IsValidHandle(handle)) {
			return; // no active weapon (the handle is 0/-1 when empty)
		}
		// if(hActiveWeapon && strstr(hActiveWeapon->GetClassname(), "c4")): the weapon handle is
		// turned into an entity index first and every lookup goes through pisexcompat, which
		// validates the entity before touching s2sdk. Calling s2sdk::GetEntityClassname directly
		// on a handle of a just-dropped weapon prints "invalid entity handle" to the server
		// console, and s2sdk entity functions take handles (index | serial << 15), never indices.
		const int weaponIndex = pisex::EntHandleToEntIndex(handle);
		if (weaponIndex <= 0) {
			return;
		}
		const std::string classname = pisex::GetEntityClassname(weaponIndex);
		if (!vip_fp::IsC4Classname(classname)) {
			return;
		}
		if (!__s2sdk_IsValidEntHandle || !s2sdk::IsValidEntHandle(handle)) {
			return;
		}
		// cC4->m_fArmedTime().SetTime(gpGlobals->curtime). The C4 is a real entity, so the write
		// goes through the handle-based schema API (the "2" pointer variant is only for
		// non-entity schema objects); changeState stays false like the original's field write.
		const pisex::GlobalVars* globals = pisex::GetCGlobalVars();
		const double curtime = globals != nullptr ? static_cast<double>(globals->curtime) : 0.0;
		s2sdk::SetEntSchemaFloat(handle, "CC4", "m_fArmedTime", curtime, false, 0);
	} catch (...) {
	}
}

plg::PluginResult FastPlantPlugin::OnPluginStart() {
	try {
		// Never fatal: only logs when the s2sdk bindings are missing.
		pisex::Init("vip_fp");

		// g_pVIPCore->VIP_RegisterFeature("fp", VIP_BOOL, TOGGLABLE) of AllPluginsLoaded.
		g_pVIPCore->VIP_RegisterFeature(kFeature, VIP_BOOL, TOGGLABLE);

		// g_pUtils->HookEvent(g_PLID, "bomb_beginplant", OnBeginPlant) of AllPluginsLoaded.
		pisex::HookEvent("bomb_beginplant", [this](const char* szName, pisex::IGameEvent* pEvent, bool bDontBroadcast) {
			OnBeginPlant(szName, pEvent, bDontBroadcast);
		});

		LogInfo(std::string("[") + kLogTag + "] ready");
	} catch (const std::exception& ex) {
		LogTaggedError(std::string("OnPluginStart failed: ") + ex.what());
	} catch (...) {
		LogTaggedError("OnPluginStart failed with an unknown exception");
	}
	return {}; // never Failed: that stops every plugify plugin
}

plg::PluginResult FastPlantPlugin::OnPluginUpdate(std::chrono::milliseconds /*dt*/) {
	try {
		pisex::Poll();
	} catch (const std::exception& ex) {
		LogTaggedError(std::string("OnPluginUpdate failed: ") + ex.what());
	} catch (...) {
		LogTaggedError("OnPluginUpdate failed with an unknown exception");
	}
	return {};
}

plg::PluginResult FastPlantPlugin::OnPluginEnd() {
	try {
		pisex::Shutdown(); // drops the bomb_beginplant hook
	} catch (...) {
		// nothing left to do at unload
	}
	return {};
}

} // namespace

PLUGIFY_PLUGIN(PLUGIN_API, &g_plugin)
