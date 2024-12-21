#include "game.hpp"

namespace UT::Game {
namespace {

struct Spell {
  RE::SpellItem* form{ nullptr };
  RE::ActorValue skill{ RE::ActorValue::kNone };
  float min{ 0.0f };
  float max{ 0.0f };
};

std::vector<Spell> Spells;
std::map<RE::FormID, std::vector<RE::ActorValue>> Skills;
std::map<RE::FormID, std::map<RE::ActorValue, std::vector<RE::BGSPerk*>>> Perks;

RE::TESForm* LF(RE::FormID id, std::string_view file)
{
  const auto form = Data->LookupForm(id, file);
  if (!form) {
    throw std::runtime_error(std::format("Could not load form: 0x{:06X} from {}", id, file));
  }
  return form;
}

template <class T>
void LF(RE::FormID id, std::string_view file, T*& object)
{
  const auto form = LF(id, file);
  if (form->GetFormType() != T::FORMTYPE) {
    const auto type = std::to_string(T::FORMTYPE);
    throw std::runtime_error(std::format("Could not cast form: 0x{:06X} from {} to {}", id, file, type));
  }
  object = form->As<T>();
}

void LP(RE::FormID id, std::string_view file, RE::ActorValue skill, std::vector<RE::FormID> forms)
{
  const auto perk = LF(id, file);
  if (perk->GetFormType() != RE::FormType::Perk) {
    throw std::runtime_error(std::format("Form is not a perk: 0x{:06X} from {}", id, file));
  }
  if (const auto name = perk->GetName(); !name || std::string_view{ name }.empty()) {
    throw std::runtime_error(std::format("Perk has no name: 0x{:06X} from {}", id, file));
  }
  for (const auto form : forms) {
    Perks[form][skill].emplace_back(perk->As<RE::BGSPerk>());
  }
}

Spell GS(RE::FormID id, std::string_view file, RE::ActorValue skill, float min, float max)
{
  auto spell = LF(id, file);
  if (spell->GetFormType() != RE::FormType::Spell) {
    throw std::runtime_error(std::format("Form is not a spell: 0x{:06X} from {}", id, file));
  }
  if (const auto name = spell->GetName(); !name || std::string_view{ name }.empty()) {
    throw std::runtime_error(std::format("Spell has no name: 0x{:06X} from {}", id, file));
  }
  return { spell->As<RE::SpellItem>(), skill, min, max };
}

void LS(RE::FormID id, std::string_view file, RE::ActorValue skill, float min, float max)
{
  Spells.emplace_back(GS(id, file, skill, min, max));
}

class ResultCallback : public RE::BSScript::IStackCallbackFunctor {
public:
  ResultCallback() = default;

  ResultCallback(std::string function, std::function<void(RE::BSScript::Variable)> callback) :
    function_(function),
    callback_(std::move(callback))
  {}

  ResultCallback(ResultCallback&& other) = default;
  ResultCallback(const ResultCallback& other) = delete;
  ResultCallback& operator=(ResultCallback&& other) = default;
  ResultCallback& operator=(const ResultCallback& other) = delete;

  void operator()(RE::BSScript::Variable result) override
  {
    if (callback_) {
      try {
        callback_(std::move(result));
      }
      catch (const std::exception& e) {
        UT_PRINT("UT: %s callback exception: %s", function_.data(), e.what());
      }
      catch (...) {
        UT_PRINT("UT: %s callback exception.", function_.data());
      }
    }
  }

  void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>& object) override {}

private:
  std::string function_;
  std::function<void(RE::BSScript::Variable)> callback_;
};

void UpdateContainerMenu(RE::BSScript::Variable result = {}) noexcept
{
  if (const auto ui = RE::UI::GetSingleton(); ui && ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
    if (const auto menu = ui->GetMenu<RE::ContainerMenu>(); menu) {
      if (const auto& data = menu->GetRuntimeData(); data.itemList) {
        data.itemList->Update();
      }
    }
  }
}

template <Mods Mod>
void LoadSkills();

template <>
void LoadSkills<Mods::Skyrim>()
{
  Skills[Guard].emplace_back(RE::ActorValue::kBlock);
  Skills[Guard].emplace_back(RE::ActorValue::kOneHanded);
  Skills[Guard].emplace_back(RE::ActorValue::kHeavyArmor);
  Skills[Guard].emplace_back(RE::ActorValue::kAlteration);

  Skills[Knight].emplace_back(RE::ActorValue::kOneHanded);
  Skills[Knight].emplace_back(RE::ActorValue::kTwoHanded);
  Skills[Knight].emplace_back(RE::ActorValue::kHeavyArmor);
  Skills[Knight].emplace_back(RE::ActorValue::kLightArmor);
  Skills[Knight].emplace_back(RE::ActorValue::kAlteration);
}

template <>
void LoadSkills<Mods::Requiem>()
{
  LoadSkills<Mods::Skyrim>();

  Skills[Warlock].emplace_back(RE::ActorValue::kOneHanded);
  Skills[Warlock].emplace_back(RE::ActorValue::kLightArmor);
  Skills[Warlock].emplace_back(RE::ActorValue::kAlteration);
  Skills[Warlock].emplace_back(RE::ActorValue::kConjuration);
  Skills[Warlock].emplace_back(RE::ActorValue::kDestruction);
  Skills[Warlock].emplace_back(RE::ActorValue::kRestoration);
  Skills[Warlock].emplace_back(RE::ActorValue::kEnchanting);
}

template <Mods Mod>
void LoadPerks();

template <>
void LoadPerks<Mods::Skyrim>()
{
  // clang-format off

  // Block
  constexpr auto Block = RE::ActorValue::kBlock;

  LP(0x0BCCAE, Skyrim, Block, { Guard });  // ShieldWall00            |   0
  LP(0x079355, Skyrim, Block, { Guard });  // ShieldWall20            |  20
  LP(0x0D8C33, Skyrim, Block, { Guard });  // QuickReflexes           |  30
  LP(0x058F68, Skyrim, Block, { Guard });  // DeflectArrows           |  30
  LP(0x058F67, Skyrim, Block, { Guard });  // PowerBashPerk           |  30
  LP(0x079356, Skyrim, Block, { Guard });  // ShieldWall40            |  40
  LP(0x058F69, Skyrim, Block, { Guard });  // ElementalProtection     |  50
  LP(0x05F594, Skyrim, Block, { Guard });  // DeadlyBash              |  50
  LP(0x079357, Skyrim, Block, { Guard });  // ShieldWall60            |  60
  LP(0x106253, Skyrim, Block, { Guard });  // BlockRunner             |  70
  LP(0x058F66, Skyrim, Block, { Guard });  // DisarmingBash           |  70
  LP(0x079358, Skyrim, Block, { Guard });  // ShieldWall80            |  80
  LP(0x058F6A, Skyrim, Block, { Guard });  // ShieldCharge            | 100

  // One-Handed
  constexpr auto OneHanded = RE::ActorValue::kOneHanded;

  LP(0x0BABE4, Skyrim, OneHanded, { Guard, Knight });  // Armsman00               |   0
  LP(0x079343, Skyrim, OneHanded, { Guard, Knight });  // Armsman20               |  20
  LP(0x052D50, Skyrim, OneHanded, { Guard, Knight });  // FightingStance          |  20
  LP(0x106256, Skyrim, OneHanded, {        Knight });  // DualFlurry30            |  30
  LP(0x05F56F, Skyrim, OneHanded, { Guard, Knight });  // Bladesman30             |  30
  LP(0x05F592, Skyrim, OneHanded, { Guard, Knight });  // BoneBreaker30           |  30
  LP(0x03FFFA, Skyrim, OneHanded, { Guard, Knight });  // HackAndSlash30          |  30
  LP(0x079342, Skyrim, OneHanded, { Guard, Knight });  // Armsman40               |  40
  LP(0x106257, Skyrim, OneHanded, {        Knight });  // DualFlurry50            |  50
  LP(0x0CB406, Skyrim, OneHanded, { Guard, Knight });  // CriticalCharge          |  50
  LP(0x03AF81, Skyrim, OneHanded, { Guard, Knight });  // SavageStrike            |  50
  LP(0x079344, Skyrim, OneHanded, { Guard, Knight });  // Armsman60               |  60
  LP(0x0C1E90, Skyrim, OneHanded, { Guard, Knight });  // Bladesman60             |  60
  LP(0x0C1E92, Skyrim, OneHanded, { Guard, Knight });  // BoneBreaker60           |  60
  LP(0x0C3678, Skyrim, OneHanded, { Guard, Knight });  // HackAndSlash60          |  60
  LP(0x106258, Skyrim, OneHanded, {        Knight });  // DualSavagery            |  70
  LP(0x079345, Skyrim, OneHanded, { Guard, Knight });  // Armsman80               |  80
  LP(0x0C1E91, Skyrim, OneHanded, { Guard, Knight });  // Bladesman90             |  90
  LP(0x0C1E93, Skyrim, OneHanded, { Guard, Knight });  // BoneBreaker90           |  90
  LP(0x0C3679, Skyrim, OneHanded, { Guard, Knight });  // HackAndSlash90          |  90
  LP(0x03AFA6, Skyrim, OneHanded, { Guard, Knight });  // ParalyzingStrike        | 100

  // Two-Handed
  constexpr auto TwoHanded = RE::ActorValue::kTwoHanded;

  LP(0x0BABE8, Skyrim, TwoHanded, { Knight });  // Barbarian00             |   0
  LP(0x079346, Skyrim, TwoHanded, { Knight });  // Barbarian20             |  20
  LP(0x052D51, Skyrim, TwoHanded, { Knight });  // ChampionsStance         |  20
  LP(0x03AF83, Skyrim, TwoHanded, { Knight });  // DeepWounds30            |  30
  LP(0x0C5C05, Skyrim, TwoHanded, { Knight });  // Limbsplitter30          |  30
  LP(0x03AF84, Skyrim, TwoHanded, { Knight });  // Skullcrusher30          |  30
  LP(0x079347, Skyrim, TwoHanded, { Knight });  // Barbarian40             |  40
  LP(0x052D52, Skyrim, TwoHanded, { Knight });  // DevastatingBlow         |  50
  LP(0x0CB407, Skyrim, TwoHanded, { Knight });  // GreatCriticalCharge     |  50
  LP(0x079348, Skyrim, TwoHanded, { Knight });  // Barbarian60             |  60
  LP(0x0C1E94, Skyrim, TwoHanded, { Knight });  // DeepWounds60            |  60
  LP(0x0C5C06, Skyrim, TwoHanded, { Knight });  // Limbsplitter60          |  60
  LP(0x0C1E96, Skyrim, TwoHanded, { Knight });  // Skullcrusher60          |  60
  LP(0x03AF9E, Skyrim, TwoHanded, { Knight });  // Sweep                   |  70
  LP(0x079349, Skyrim, TwoHanded, { Knight });  // Barbarian80             |  80
  LP(0x0C1E95, Skyrim, TwoHanded, { Knight });  // DeepWounds90            |  90
  LP(0x0C5C07, Skyrim, TwoHanded, { Knight });  // Limbsplitter90          |  90
  LP(0x0C1E97, Skyrim, TwoHanded, { Knight });  // Skullcrusher90          |  90
  LP(0x03AFA7, Skyrim, TwoHanded, { Knight });  // Warmaster               | 100

  // Heavy Armor
  constexpr auto HeavyArmor = RE::ActorValue::kHeavyArmor;

  LP(0x0BCD2A, Skyrim, HeavyArmor, { Guard, Knight });  // Juggernaut00            |   0
  LP(0x07935E, Skyrim, HeavyArmor, { Guard, Knight });  // Juggernaut20            |  20
  LP(0x058F6E, Skyrim, HeavyArmor, { Guard, Knight });  // FistsOfSteel            |  30
  LP(0x058F6F, Skyrim, HeavyArmor, { Guard, Knight });  // WellFitted              |  30
  LP(0x079361, Skyrim, HeavyArmor, { Guard, Knight });  // Juggernaut40            |  40
  LP(0x0BCD2B, Skyrim, HeavyArmor, { Guard, Knight });  // Cushioned               |  50
  LP(0x058F6C, Skyrim, HeavyArmor, { Guard, Knight });  // TowerOfStrength         |  50
  LP(0x079362, Skyrim, HeavyArmor, { Guard, Knight });  // Juggernaut60            |  60
  LP(0x058F6D, Skyrim, HeavyArmor, { Guard, Knight });  // Conditioning            |  70
  LP(0x107832, Skyrim, HeavyArmor, { Guard, Knight });  // MatchingSetHeavy        |  70
  LP(0x079374, Skyrim, HeavyArmor, { Guard, Knight });  // Juggernaut80            |  80
  LP(0x105F33, Skyrim, HeavyArmor, { Guard, Knight });  // ReflectBlows            | 100

  // Light Armor
  constexpr auto LightArmor = RE::ActorValue::kLightArmor;

  LP(0x0BE123, Skyrim, LightArmor, { Knight });  // AgileDefender00         |   0
  LP(0x079376, Skyrim, LightArmor, { Knight });  // AgileDefender20         |  20
  LP(0x051B1B, Skyrim, LightArmor, { Knight });  // CustomFit               |  30
  LP(0x079389, Skyrim, LightArmor, { Knight });  // AgileDefender40         |  40
  LP(0x051B1C, Skyrim, LightArmor, { Knight });  // Unhindered              |  50
  LP(0x079391, Skyrim, LightArmor, { Knight });  // AgileDefender60         |  60
  LP(0x105F22, Skyrim, LightArmor, { Knight });  // WindWalker              |  60
  LP(0x051B17, Skyrim, LightArmor, { Knight });  // MatchingSet             |  70
  LP(0x079392, Skyrim, LightArmor, { Knight });  // AgileDefender80         |  80
  LP(0x107831, Skyrim, LightArmor, { Knight });  // DeftMovement            | 100

  // Alteration
  constexpr auto Alteration = RE::ActorValue::kAlteration;

  LP(0x053128, Skyrim, Alteration, { Guard, Knight });  //  MagicResistance30       |  30
  LP(0x053129, Skyrim, Alteration, { Guard, Knight });  //  MagicResistance50       |  50
  LP(0x05312A, Skyrim, Alteration, { Guard, Knight });  //  MagicResistance70       |  70
  LP(0x0581F7, Skyrim, Alteration, { Guard, Knight });  //  atronach                | 100

  // clang-format on
}

template <>
void LoadPerks<Mods::Requiem>()
{
  // clang-format off

  // Block
  constexpr auto Block = RE::ActorValue::kBlock;

  LP(0x0BCCAE, Skyrim, Block, { Guard });  // REQ_Block_ImprovedBlocking             |   0
  LP(0x058F68, Skyrim, Block, { Guard });  // REQ_Block_StrongGrip                   |  15
  LP(0x079355, Skyrim, Block, { Guard });  // REQ_Block_ExperiencedBlocking          |  20
  LP(0x058F67, Skyrim, Block, { Guard });  // REQ_Block_PowerfulBashes               |  25
  LP(0x058F69, Skyrim, Block, { Guard });  // REQ_Block_ElementalProtection          |  50
  LP(0x05F594, Skyrim, Block, { Guard });  // REQ_Block_OverpoweringBashes           |  50
  LP(0x106253, Skyrim, Block, { Guard });  // REQ_Block_DefensiveStance              |  75
  LP(0x058F66, Skyrim, Block, { Guard });  // REQ_Block_DisarmingBash                |  75
  LP(0x058F6A, Skyrim, Block, { Guard });  // REQ_Block_UnstoppableCharge            | 100

  // One-Handed
  constexpr auto OneHanded = RE::ActorValue::kOneHanded;

  LP(0x0BABE4, Skyrim,  OneHanded, { Guard, Knight, Warlock });  // REQ_OneHanded_WeaponMastery1           |   0
  LP(0x079343, Skyrim,  OneHanded, { Guard, Knight, Warlock });  // REQ_OneHanded_WeaponMastery2           |   0
  LP(0x052D50, Skyrim,  OneHanded, { Guard, Knight, Warlock });  // REQ_OneHanded_PenetratingStrikes       |  20
  LP(0xAD399A, Requiem, OneHanded, { Guard, Knight, Warlock });  // REQ_OneHanded_DaggerFocus1             |  25
  LP(0x03FFFA, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_WarAxeFocus1             |  25
  LP(0x05F592, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_MaceFocus1               |  25
  LP(0x05F56F, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_SwordFocus1              |  25
  LP(0x106256, Skyrim,  OneHanded, {        Knight          });  // REQ_OneHanded_Flurry1                  |  25
  LP(0xAD3999, Requiem, OneHanded, { Guard, Knight, Warlock });  // REQ_OneHanded_DaggerFocus2             |  50
  LP(0x0C3678, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_WarAxeFocus2             |  50
  LP(0x0C1E92, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_MaceFocus2               |  50
  LP(0x0C1E90, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_SwordFocus2              |  50
  LP(0x03AF81, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_PowerfulStrike           |  50
  LP(0x0CB406, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_PowerfulCharge           |  50
  LP(0x106257, Skyrim,  OneHanded, {        Knight          });  // REQ_OneHanded_Flurry2                  |  50
  LP(0xAD3998, Requiem, OneHanded, { Guard, Knight, Warlock });  // REQ_OneHanded_DaggerFocus3             |  75
  LP(0x0C3679, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_WarAxeFocus3             |  75
  LP(0x0C1E93, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_MaceFocus3               |  75
  LP(0x0C1E91, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_SwordFocus3              |  75
  LP(0x106258, Skyrim,  OneHanded, {        Knight          });  // REQ_OneHanded_StormOfSteel             |  75
  LP(0x03AFA6, Skyrim,  OneHanded, { Guard, Knight          });  // REQ_OneHanded_StunningCharge           | 100

  // Two-Handed
  constexpr auto TwoHanded = RE::ActorValue::kTwoHanded;

  LP(0x0BABE8, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_GreatWeaponMastery1      |   0
  LP(0x079346, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_GreatWeaponMastery2      |   0
  LP(0x052D51, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_BarbaricMight            |  20
  LP(0xADDFB0, Requiem, TwoHanded, { Knight });  // REQ_TwoHanded_QuarterstaffFocus1       |  25
  LP(0x0C5C05, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_BattleAxeFocus1          |  25
  LP(0x03AF83, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_GreatswordFocus1         |  25
  LP(0x03AF84, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_WarhammerFocus1          |  25
  LP(0xADDFB1, Requiem, TwoHanded, { Knight });  // REQ_TwoHanded_QuarterstaffFocus2       |  50
  LP(0x0C5C06, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_BattleAxeFocus2          |  50
  LP(0x0C1E94, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_GreatswordFocus2         |  50
  LP(0x0C1E96, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_WarhammerFocus2          |  50
  LP(0x0CB407, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_DevastatingCharge        |  50
  LP(0x052D52, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_DevastatingStrike        |  50
  LP(0xADDFB2, Requiem, TwoHanded, { Knight });  // REQ_TwoHanded_QuarterstaffFocus3       |  75
  LP(0x0C5C07, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_BattleAxeFocus3          |  75
  LP(0x0C1E95, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_GreatswordFocus3         |  75
  LP(0x0C1E97, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_WarhammerFocus3          |  75
  LP(0x03AF9E, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_Cleave                   |  75
  LP(0x03AFA7, Skyrim,  TwoHanded, { Knight });  // REQ_TwoHanded_DevastatingCleave        | 100
  LP(0x182F9B, Requiem, TwoHanded, { Knight });  // REQ_TwoHanded_MightyStrike             | 100

  // Heavy Armor
  constexpr auto HeavyArmor = RE::ActorValue::kHeavyArmor;

  LP(0x0BCD2A, Skyrim, HeavyArmor, { Guard, Knight });  // REQ_HeavyArmor_Conditioning            |   0
  LP(0x07935E, Skyrim, HeavyArmor, { Guard, Knight });  // REQ_HeavyArmor_RelentlessOnslaught     |  20
  LP(0x058F6F, Skyrim, HeavyArmor, { Guard, Knight });  // REQ_HeavyArmor_CombatTraining          |  25
  LP(0x058F6C, Skyrim, HeavyArmor, { Guard, Knight });  // REQ_HeavyArmor_Fortitude               |  50
  LP(0x107832, Skyrim, HeavyArmor, { Guard, Knight });  // REQ_HeavyArmor_PowerOfTheCombatant     |  75
  LP(0x105F33, Skyrim, HeavyArmor, { Guard, Knight });  // REQ_HeavyArmor_Juggernaut              | 100

  // Evasion
  constexpr auto Evasion = RE::ActorValue::kLightArmor;

  LP(0x0BE123, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_Agility                    |   0
  LP(0x079376, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_Dodge                      |  20
  LP(0x051B1B, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_Finesse                    |  25
  LP(0x18A66F, Requiem, Evasion, {         Warlock });  // REQ_Evasion_AgileSpellcasting          |  30
  LP(0x051B1C, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_Dexterity                  |  50
  LP(0x18F5A8, Requiem, Evasion, { Knight, Warlock });  // REQ_Evasion_VexingFlanker              |  50
  LP(0x105F22, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_WindWalker                 |  75
  LP(0x051B17, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_CombatReflexes             |  75
  LP(0x107831, Skyrim,  Evasion, { Knight, Warlock });  // REQ_Evasion_MeteoricReflexes           | 100

  // Alteration
  constexpr auto Alteration = RE::ActorValue::kAlteration;

  LP(0x0D7999, Skyrim,  Alteration, {                Warlock });  // REQ_Alteration_ImprovedMageArmor       |  25
  LP(0x053128, Skyrim,  Alteration, { Guard, Knight, Warlock });  // REQ_Alteration_MagicResistance1        |  25
  LP(0x0581FC, Skyrim,  Alteration, {                Warlock });  // REQ_Alteration_Stability               |  50
  LP(0x053129, Skyrim,  Alteration, { Guard, Knight, Warlock });  // REQ_Alteration_MagicResistance2        |  50
  LP(0x21792B, Requiem, Alteration, {                Warlock });  // REQ_Alteration_MetamagicalThesis       |  75
  LP(0x21792A, Requiem, Alteration, {                Warlock });  // REQ_Alteration_SpellArmor              |  75
  LP(0x05312A, Skyrim,  Alteration, { Guard, Knight, Warlock });  // REQ_Alteration_MagicResistance3        |  75
  LP(0x21792C, Requiem, Alteration, {                Warlock });  // REQ_Alteration_MetamagicalEmpowerment  | 100
  LP(0x0581F7, Skyrim,  Alteration, { Guard, Knight, Warlock });  // REQ_Alteration_MagicalAbsorption       | 100

  // Conjuration
  constexpr auto Conjuration = RE::ActorValue::kConjuration;

  LP(0x105F30, Skyrim,  Conjuration, { Warlock });  // REQ_Conjuration_StabilizedBinding      |  25
  LP(0xAD385A, Requiem, Conjuration, { Warlock });  // REQ_Conjuration_SpiritualBinding       |  35
  LP(0x0CB419, Skyrim,  Conjuration, { Warlock });  // REQ_Conjuration_ExtendedBinding        |  50
  LP(0x0CB41A, Skyrim,  Conjuration, { Warlock });  // REQ_Conjuration_ElementalBinding       |  75

  // Destruction
  constexpr auto Destruction = RE::ActorValue::kDestruction;

  LP(0x0581E7, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Pyromancy1             |  25
  LP(0x0581EA, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Cyromancy1             |  25
  LP(0x058200, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Electromancy1          |  25
  LP(0x10FCF8, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Pyromancy2             |  50
  LP(0x10FCF9, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Cyromancy2             |  50
  LP(0x10FCFA, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Electromancy2          |  50
  LP(0x0153D2, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Impact                 |  50
  LP(0x0F392E, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_Cremation              |  75
  LP(0x0F3933, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_DeepFreeze             |  75
  LP(0x0F3F0E, Skyrim,  Destruction, { Warlock });  // REQ_Destruction_ElectrostaticDischarge |  75
  LP(0x179121, Requiem, Destruction, { Warlock });  // REQ_Destruction_FireMastery            | 100
  LP(0x179123, Requiem, Destruction, { Warlock });  // REQ_Destruction_FrostMastery           | 100
  LP(0x179124, Requiem, Destruction, { Warlock });  // REQ_Destruction_LightningMastery       | 100

  // Restoration
  constexpr auto Restoration = RE::ActorValue::kRestoration;

  LP(0x0581F4, Skyrim, Restoration, { Warlock });  // REQ_Restoration_FocusedMind            |  25
  LP(0x068BCC, Skyrim, Restoration, { Warlock });  // REQ_Restoration_ImprovedWards          |  75

  // Enchanting
  constexpr auto Enchanting = RE::ActorValue::kEnchanting;

  LP(0x0BEE97, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_EnchantersInsight1      |   0
  LP(0x0C367C, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_EnchantersInsight2      |  20
  LP(0x058F80, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_ElementalLore           |  25
  LP(0x058F7C, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_SoulGemMastery          |  25
  LP(0x058F81, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_CorpusLore              |  50
  LP(0x058F7E, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_ArcaneExperimentation   |  50
  LP(0x058F82, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_SkillLore               |  75
  LP(0x058F7D, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_ArtificersInsight       |  75
  LP(0x058F7F, Skyrim, Enchanting, { Warlock });  // REQ_Enchanting_EnchantmentMastery      | 100

  // clang-format on
}

template <Mods Mod>
void LoadSpells();

template <>
void LoadSpells<Mods::Requiem>()
{
  // clang-format off
  constexpr auto Conjuration = RE::ActorValue::kConjuration;
  constexpr auto Destruction = RE::ActorValue::kDestruction;
  constexpr auto Restoration = RE::ActorValue::kRestoration;

  // Conjuration
  LS(0x0204C5, Skyrim, Conjuration,  75.0f, 100.0f);  // Summon Storm Atronach

  // Destruction
  LS(0x012FCD, Skyrim, Destruction,   0.0f,  74.0f);  // Flames
  LS(0x02B96B, Skyrim, Destruction,   0.0f,  74.0f);  // Frostbite
  LS(0x02DD2A, Skyrim, Destruction,   0.0f,  74.0f);  // Sparks
  LS(0x012FD0, Skyrim, Destruction,  25.0f,  74.0f);  // Firebolt
  LS(0x02B96C, Skyrim, Destruction,  25.0f,  74.0f);  // Ice Spike
  LS(0x02DD29, Skyrim, Destruction,  25.0f,  74.0f);  // Lightning Bolt
  LS(0x01C789, Skyrim, Destruction,  50.0f, 100.0f);  // Fireball
  LS(0x045F9C, Skyrim, Destruction,  50.0f, 100.0f);  // Ice Storm
  LS(0x045F9D, Skyrim, Destruction,  50.0f, 100.0f);  // Chain Lightning
  LS(0x10F7ED, Skyrim, Destruction,  75.0f, 100.0f);  // Incinerate
  LS(0x10F7EC, Skyrim, Destruction,  75.0f, 100.0f);  // Icy Spear
  LS(0x10F7EE, Skyrim, Destruction,  75.0f, 100.0f);  // Thunderbolt

  // Restoration
  LS(0x225F3B, Requiem, Restoration,   0.0f,  24.0f);  // Arcane Ward (Rank I)
  LS(0x013018, Skyrim,  Restoration,  25.0f,  49.0f);  // Arcane Ward (Rank II)
  LS(0x0211F1, Skyrim,  Restoration,  50.0f, 100.0f);  // Arcane Ward (Rank III)
  LS(0x0211F0, Skyrim,  Restoration,  75.0f, 100.0f);  // Arcane Ward (Rank IV)

  // clang-format on
}

}  // namespace

void Load()
{
  // Get singletons.
  Papyrus = SKSE::GetPapyrusInterface();
  if (!Papyrus) {
    throw std::runtime_error("Could not get papyrus interface.");
  }

  VirtualMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!VirtualMachine) {
    throw std::runtime_error("Could not get virtual machine.");
  }

  Player = RE::PlayerCharacter::GetSingleton();
  if (!Player) {
    throw std::runtime_error("Could not get player singleton.");
  }

  Data = RE::TESDataHandler::GetSingleton();
  if (!Data) {
    throw std::runtime_error("Could not get TES data handler.");
  }

  // Load actors.
  Guard = LF(0x000031, Trinity)->GetFormID();
  Knight = LF(0x000032, Trinity)->GetFormID();
  Warlock = LF(0x000033, Trinity)->GetFormID();

  // Load packages.
  LF(0xF00004, Trinity, Heal);
  LF(0xF00005, Trinity, HealGuard);
  LF(0xF00006, Trinity, HealKnight);
  LF(0xF00007, Trinity, HealSelf);

  // Load skills, perks and spells forms.
  if (Data->GetModIndex(Requiem)) {
    Mod = Mods::Requiem;
    LoadSkills<Mods::Requiem>();
    LoadPerks<Mods::Requiem>();
    LoadSpells<Mods::Requiem>();
  } else {
    Mod = Mods::Skyrim;
    LoadSkills<Mods::Skyrim>();
    LoadPerks<Mods::Skyrim>();
  }
}

void Initialize(RE::FormID id, RE::Actor* actor) noexcept
{
  auto base = actor->GetActorBase();
  if (!base) {
    UT_PRINT("UT: [%s] Could not get actor base.", GetName(id));
    return;
  }

  auto pvo = Player->AsActorValueOwner();
  if (!pvo) {
    UT_PRINT("UT: [%s] Could not get player actor value owner.", GetName(id));
    return;
  }

  auto avo = actor->AsActorValueOwner();
  if (!avo) {
    UT_PRINT("UT: [%s] Could not get %s actor value owner.", GetName(id), actor->GetName());
    return;
  }

  const auto& skills = Skills[id];
  boost::container::flat_map<RE::ActorValue, float> skillValues;
  skillValues.reserve(skills.size());

  for (const auto skill : skills) {
    const auto pv = std::min(pvo->GetBaseActorValue(skill), 100.0f);
    const auto av = std::min(avo->GetBaseActorValue(skill), 100.0f);
    if (pv > av + 0.5f) {
      avo->SetBaseActorValue(skill, pv);
      UT_TRACE("UT: [%s] SKIL %3.0f -> %3.0f %s", GetName(id), av, pv, std::to_string(skill).data());
    }
    skillValues[skill] = pv;
  }

  int hasPerks = 0;
  int addPerks = 0;
  for (const auto& perks : Perks[id]) {
    for (const auto perk : perks.second) {
      if (actor->HasPerk(perk)) {
        hasPerks++;
        continue;
      }
      if (!perk->perkConditions.IsTrue(actor, actor)) {
        UT_DEBUG(
          "UT: [%s] PERK %s: Conditions not met: %08X %s",
          GetName(id),
          std::to_string(perks.first).data(),
          perk->GetFormID(),
          perk->GetName());
        break;
      }
      if (!base->AddPerk(perk, 1)) {
        UT_PRINT(
          "UT: [%s] PERK %s: Could not add: %08X %s",
          GetName(id),
          std::to_string(perks.first).data(),
          perk->GetFormID(),
          perk->GetName());
        break;
      }
      addPerks++;
#if !defined(NDEBUG) && UT_DEBUG_PERKS
      UT_DEBUG(
        "UT: [%s] PERK %s: %08X %s",
        GetName(id),
        std::to_string(perks.first).data(),
        perk->GetFormID(),
        perk->GetName());
#endif
    }
  }

#if !defined(NDEBUG) && UT_DEBUG_PERKS
  const auto maxPerks = std::accumulate(Perks[id].begin(), Perks[id].end(), 0, [](int v, const auto& e) {
    return v + static_cast<int>(e.second.size());
  });
  UT_DEBUG("UT: [%s] PERK %d/%d (%d added)", GetName(id), hasPerks + addPerks, maxPerks, addPerks);
#endif

  if (id == Warlock) {
    UT_TRACE("UT: [W] Updating %u spells ...", static_cast<unsigned>(Spells.size()));
    for (const auto& spell : Spells) {
#if !defined(NDEBUG) && UT_DEBUG_SPELL
      const auto form = spell.form->GetFormID();
      const auto name = spell.form->GetName();
      const auto info = std::to_string(spell.skill);
      const auto skill = skillValues[spell.skill];
      if (skill < spell.min) {
        UT_DEBUG("UT: [W] SPEL %.0f < %.0f %s: %08X %s", skill, spell.min, info.data(), form, name);
        continue;
      }
      if (skill > spell.max) {
        UT_DEBUG("UT: [W] SPEL %.0f > %.0f %s: %08X %s", skill, spell.max, info.data(), form, name);
        continue;
      }
      if (actor->HasSpell(spell.form)) {
        UT_DEBUG("UT: [W] SPEL Known: %08X %s", form, name);
        continue;
      }
      if (!Player->HasSpell(spell.form)) {
        UT_DEBUG("UT: [W] SPEL Unknown: %08X %s", form, name);
        continue;
      }
      if (!actor->AddSpell(spell.form)) {
        UT_PRINT("UT: [W] SPEL Could not add: %08X %s", form, name);
      }
      UT_DEBUG("UT: [W] SPEL %08X %s", form, name);
#else
      if (const auto skill = skillValues[spell.skill]; skill >= spell.min && skill <= spell.max) {
        const auto form = spell.form;
        if (actor->HasSpell(form) || !Player->HasSpell(form)) {
          continue;
        }
        if (!actor->AddSpell(form)) {
          UT_PRINT("UT: [%s] SPEL Could not add: %08X %s", GetName(id), form->GetFormID(), form->GetName());
          break;
        }
        UT_TRACE("UT: [%s] SPEL %08X %s", GetName(id), form->GetFormID(), form->GetName());
      }
#endif
    }
  }
}

void Equip(RE::FormID id, RE::Actor* actor, RE::TESBoundObject* object) noexcept
{
  // Only equip armor.
  if (object->GetFormType() != RE::FormType::Armor) {
    UT_TRACE("UT: [%s] Item is not armor.", GetName(id));
    return;
  }
  const auto armor = object->As<RE::TESObjectARMO>();
  if (!armor) {
    UT_PRINT("UT: [%s] Could not get armor.", GetName(id));
    return;
  }

  // Unequip armor with conflicting slot masks.
  if (const auto mask = static_cast<unsigned>(armor->GetSlotMask())) {
    if (const auto manager = RE::ActorEquipManager::GetSingleton()) {
      for (const auto& e : actor->GetInventory()) {
        if (const auto data = e.second.second.get(); data && data->IsWorn()) {
          if (e.first && e.first->GetFormType() == RE::FormType::Armor) {
            if (const auto equipped = e.first->As<RE::TESObjectARMO>()) {
              if (mask & static_cast<unsigned>(equipped->GetSlotMask())) {
                if (const auto extras = data->extraLists; extras && !extras->empty()) {
                  UT_DEBUG("UT: [%s] R: %s", GetName(id), e.first->GetName());
                  manager->UnequipObject(actor, e.first, extras->front(), 1, nullptr, false, false, false, true);
                }
              }
            }
          }
        }
      }
    }
  }

  // Equip armor.
  const auto policy = VirtualMachine->GetObjectHandlePolicy();
  if (!policy) {
    UT_PRINT("UT: [%s] Could not get object handle policy.", GetName(id));
    return;
  }

  const auto handle = policy->GetHandleForObject(actor->GetFormType(), actor);
  if (handle == policy->EmptyHandle()) {
    UT_PRINT("UT: [%s] Could not get object handle: %08X UT_Actor", GetName(id), actor->GetFormID());
    return;
  }

  struct Script {
    RE::BSFixedString name = "Actor";
    RE::BSFixedString function = "EquipItemEx";
    RE::BSTSmartPointer<RE::BSScript::Object> object;
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result{
      new ResultCallback{ "Actor.EquipItemEx", UpdateContainerMenu }
    };
  } script;

  if (!VirtualMachine->FindBoundObject(handle, script.name.c_str(), script.object)) {
    UT_PRINT("UT: [%s] Could not find bound script object: %08X Actor", GetName(id), actor->GetFormID());
    return;
  }

  auto args = RE::MakeFunctionArguments(static_cast<RE::TESForm*>(object), 0, true, true);
  if (!VirtualMachine->DispatchMethodCall1(script.object, script.function, args, script.result)) {
    UT_PRINT("UT: [%s] Could not call script function: %08X Actor.EquipItemEx", GetName(id), actor->GetFormID());
    return;
  }

  UT_DEBUG("UT: [%s] E: %s", GetName(id), object->GetName());
}

void Unequip(RE::FormID id, RE::Actor* actor, RE::TESBoundObject* object, RE::ExtraDataList* extra) noexcept
{
  // Only unequip armor.
  if (object->GetFormType() != RE::FormType::Armor) {
    UT_TRACE("UT: [%s] Item is not armor.", GetName(id));
    return;
  }

  // Get actor equip manager.
  const auto manager = RE::ActorEquipManager::GetSingleton();
  if (!manager) {
    UT_PRINT("UT: [%s] Could not get actor equip manager.", GetName(id));
    return;
  }

  // Unequip armor.
  UT_DEBUG("UT: [%s] U: %s", GetName(id), object->GetName());
  manager->UnequipObject(actor, object, extra);

  // Update container menu.
  UpdateContainerMenu();
}

bool CanEquip(RE::FormID id, RE::TESBoundObject* object) noexcept
{
  if (object->GetFormType() == RE::FormType::Armor) {
    if (const auto armor = object->As<RE::TESObjectARMO>()) {
      if (armor->IsShield()) {
        return id == Guard;
      }
      switch (armor->GetArmorType()) {
      case RE::BGSBipedObjectForm::ArmorType::kLightArmor:
        return id == Knight;
      case RE::BGSBipedObjectForm::ArmorType::kHeavyArmor:
        return id == Guard || id == Knight;
      case RE::BGSBipedObjectForm::ArmorType::kClothing:
        return true;
      }
    }
  }
  return false;
}

void AddPackageOverride(
  RE::FormID id,
  RE::Actor* actor,
  RE::TESPackage* package,
  int priority,
  bool force,
  std::function<void(RE::BSScript::Variable)> callback) noexcept
{
  const auto policy = VirtualMachine->GetObjectHandlePolicy();
  if (!policy) {
    UT_PRINT("UT: [%s] Could not get object handle policy.", GetName(id));
    return;
  }

  const auto handle = policy->GetHandleForObject(actor->GetFormType(), actor);
  if (handle == policy->EmptyHandle()) {
    UT_PRINT("UT: [%s] Could not get object handle: %08X UT_Actor", GetName(id), actor->GetFormID());
    return;
  }

  struct Script {
    RE::BSFixedString name = "ActorUtil";
    RE::BSFixedString function = "AddPackageOverride";
    RE::BSTSmartPointer<RE::BSScript::Object> object;
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
  } script;

  if (callback) {
    script.result.reset(new ResultCallback("ActorUtil.AddPackageOverride", std::move(callback)));
  }

  auto flag = force ? 1 : 0;
  auto args = RE::MakeFunctionArguments(std::move(actor), std::move(package), std::move(priority), std::move(flag));
  if (!VirtualMachine->DispatchStaticCall(script.name, script.function, args, script.result)) {
    UT_PRINT("UT: [%s] Could not call script function: ActorUtil.AddPackageOverride", GetName(id));
    return;
  }
}

void RemovePackageOverride(
  RE::FormID id,
  RE::Actor* actor,
  RE::TESPackage* package,
  std::function<void(RE::BSScript::Variable)> callback) noexcept
{
  const auto policy = VirtualMachine->GetObjectHandlePolicy();
  if (!policy) {
    UT_PRINT("UT: [%s] Could not get object handle policy.", GetName(id));
    return;
  }

  const auto handle = policy->GetHandleForObject(actor->GetFormType(), actor);
  if (handle == policy->EmptyHandle()) {
    UT_PRINT("UT: [%s] Could not get object handle: %08X UT_Actor", GetName(id), actor->GetFormID());
    return;
  }

  struct Script {
    RE::BSFixedString name = "ActorUtil";
    RE::BSFixedString function = "RemovePackageOverride";
    RE::BSTSmartPointer<RE::BSScript::Object> object;
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
  } script;

  if (callback) {
    script.result.reset(new ResultCallback("ActorUtil.RemovePackageOverride", std::move(callback)));
  }

  auto args = RE::MakeFunctionArguments(std::move(actor), std::move(package));
  if (!VirtualMachine->DispatchStaticCall(script.name, script.function, args, script.result)) {
    UT_PRINT("UT: [%s] Could not call script function: ActorUtil.RemovePackageOverride", GetName(id));
    return;
  }
}

void ClearPackageOverride(RE::FormID id, RE::Actor* actor, std::function<void(RE::BSScript::Variable)> callback) noexcept
{
  const auto policy = VirtualMachine->GetObjectHandlePolicy();
  if (!policy) {
    UT_PRINT("UT: [%s] Could not get object handle policy.", GetName(id));
    return;
  }

  const auto handle = policy->GetHandleForObject(actor->GetFormType(), actor);
  if (handle == policy->EmptyHandle()) {
    UT_PRINT("UT: [%s] Could not get object handle: %08X UT_Actor", GetName(id), actor->GetFormID());
    return;
  }

  struct Script {
    RE::BSFixedString name = "ActorUtil";
    RE::BSFixedString function = "ClearPackageOverride";
    RE::BSTSmartPointer<RE::BSScript::Object> object;
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
  } script;

  if (callback) {
    script.result.reset(new ResultCallback("ActorUtil.ClearPackageOverride", std::move(callback)));
  }

  auto args = RE::MakeFunctionArguments(std::move(actor));
  if (!VirtualMachine->DispatchStaticCall(script.name, script.function, args, script.result)) {
    UT_PRINT("UT: [%s] Could not call script function: ActorUtil.ClearPackageOverride", GetName(id));
    return;
  }
}

float GetHealth(RE::Actor* actor) noexcept
{
  if (!actor) {
    return 1.0f;
  }
  if (actor->IsDead()) {
    return 0.0f;
  }
  if (const auto avo = actor->AsActorValueOwner()) {
    const auto cur = avo->GetActorValue(RE::ActorValue::kHealth);
    const auto max = avo->GetPermanentActorValue(RE::ActorValue::kHealth);
    const auto tmp = actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kHealth);
    return cur / std::max(max + tmp, 1.0f);
  }
#if !defined(NDEBUG) && UT_DEBUG_TRACE
  if (const auto base = actor->GetActorBase()) {
    UT_TRACE("UT: [%s] Coult not get actor value owner.", GetName(base->GetFormID()));
  } else {
    UT_TRACE("UT: [U] Coult not get actor value owner.");
  }
#endif
  return 1.0f;
}

}  // namespace UT::Game
