#pragma once

#define UT_DEBUG_TRACE 0
#define UT_DEBUG_PERKS 0
#define UT_DEBUG_SPELL 0

#define UT_PRINT RE::ConsoleLog::GetSingleton()->Print

#ifndef NDEBUG
#define UT_DEBUG UT_PRINT
#if defined(UT_DEBUG_TRACE) && UT_DEBUG_TRACE
#define UT_TRACE UT_PRINT
#else
#define UT_TRACE(...)
#endif
#else
#define UT_DEBUG(...)
#define UT_TRACE(...)
#endif

namespace UT::Game {

enum class Mods {
  Skyrim,
  Requiem,
};

inline Mods Mod = Mods::Skyrim;

constexpr std::string_view Skyrim{ "Skyrim.esm" };
constexpr std::string_view Dawnguard{ "Dawnguard.esm" };
constexpr std::string_view Requiem{ "Requiem.esp" };
constexpr std::string_view Trinity{ "Undead Trinity.esp" };

inline const SKSE::PapyrusInterface* Papyrus{ nullptr };

inline RE::BSScript::Internal::VirtualMachine* VirtualMachine{ nullptr };
inline RE::PlayerCharacter* Player{ nullptr };
inline RE::TESDataHandler* Data{ nullptr };

// Guard
// =============================================================================================
// Attrib: 2 Health, 1 Stamina
// Skills: 4 Heavy Armor, 3 Block, 2 One-Handed, 1 Alteration
// Equips: Heavy Armor, One-Handed, Shield
inline RE::FormID Guard{ 0x000000 };

// Knight
// =============================================================================================
// Attrib: 1 Health, 1 Stamina
// Skills: 3 One-Handed, 3 Two-Handed, 2 Heavy Armor, 2 Light Armor, 1 Alteration
// Equips: Light Armor, Heavy Armor, One-Handed (Dual Wield), Two-Handed
inline RE::FormID Knight{ 0x000000 };

// Warlock
// =============================================================================================
// Attrib: 3 Magicka, 2 Health, 1 Stamina
// Skills: 4 Destruction, 3 Conjuration, 2 Alteration, 1 One-Handed, 1 Restoration, 1 Enchanting
// Skills: 1 Evasion (Requiem)
// Equips: Cloth, One-Handed (Dagger), Staff, Spell
inline RE::FormID Warlock{ 0x000000 };

inline RE::TESPackage* Heal{ nullptr };
inline RE::TESPackage* HealGuard{ nullptr };
inline RE::TESPackage* HealKnight{ nullptr };
inline RE::TESPackage* HealSelf{ nullptr };

void Load();
void Initialize(RE::FormID id, RE::Actor* actor) noexcept;

void Equip(RE::FormID id, RE::Actor* actor, RE::TESBoundObject* object) noexcept;
void Unequip(RE::FormID id, RE::Actor* actor, RE::TESBoundObject* object, RE::ExtraDataList* extra) noexcept;

bool CanEquip(RE::FormID id, RE::TESBoundObject* object) noexcept;

void AddPackageOverride(
  RE::FormID id,
  RE::Actor* actor,
  RE::TESPackage* package,
  int priority = 30,
  bool force = false,
  std::function<void(RE::BSScript::Variable)> callback = {}) noexcept;

void RemovePackageOverride(
  RE::FormID id,
  RE::Actor* actor,
  RE::TESPackage* package,
  std::function<void(RE::BSScript::Variable)> callback = {}) noexcept;

void ClearPackageOverride(
  RE::FormID id,
  RE::Actor* actor,
  std::function<void(RE::BSScript::Variable)> callback = {}) noexcept;

float GetHealth(RE::Actor* actor) noexcept;

constexpr const char* GetName(Mods mod) noexcept
{
  switch (mod) {
  case Mods::Skyrim:
    return "Skyrim";
  case Mods::Requiem:
    return "Requiem";
  }
  return "Unknown";
}

constexpr const char* GetName(RE::FormID id) noexcept
{
  if (id == Guard) {
    return "G";
  }
  if (id == Knight) {
    return "K";
  }
  if (id == Warlock) {
    return "W";
  }
  return id == 0x00000007 ? "P" : "U";
}

constexpr const char* GetName(RE::TESPackage* package) noexcept
{
  if (package == Heal) {
    return "Heal";
  }
  if (package == HealGuard) {
    return "Heal Guard";
  }
  if (package == HealKnight) {
    return "Heal Knight";
  }
  if (package == HealSelf) {
    return "Heal Self";
  }
  return "Unknown";
}

constexpr const char* GetName(RE::ACTOR_COMBAT_STATE state) noexcept
{
  switch (state) {
  case RE::ACTOR_COMBAT_STATE::kNone:
    return "None";
  case RE::ACTOR_COMBAT_STATE::kCombat:
    return "Combat";
  case RE::ACTOR_COMBAT_STATE::kSearching:
    return "Searching";
  }
  return "Unknown";
}

}  // namespace UT::Game
