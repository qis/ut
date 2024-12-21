#include <game.hpp>
#include <trinity.hpp>
#include <version.h>

namespace UT {

class Manager final :
  public RE::BSTEventSink<RE::InputEvent*>,
  public RE::BSTEventSink<RE::TESCombatEvent>,
  public RE::BSTEventSink<RE::TESHitEvent> {
private:
  Manager() noexcept = default;
  Manager(Manager&&) = delete;
  Manager(const Manager&) = delete;
  Manager& operator=(const Manager&) = delete;
  Manager& operator=(Manager&&) = delete;

public:
  static void Listener(SKSE::MessagingInterface::Message* message) noexcept
  {
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
      if (auto manager = GetSingleton(); manager->Initialize()) {
        constexpr int major = PROJECT_VERSION_MAJOR;
        constexpr int minor = PROJECT_VERSION_MINOR;
        constexpr int patch = PROJECT_VERSION_PATCH;
        UT_PRINT("Undead Trinity (%s) %d.%d.%d loaded.", Game::GetName(Game::Mod), major, minor, patch);
      }
      break;
    case SKSE::MessagingInterface::kPreLoadGame:
      if (auto manager = GetSingleton(); manager->initialized_) {
        manager->OnPreLoadGame();
      }
      break;
    case SKSE::MessagingInterface::kPostLoadGame:
      if (auto manager = GetSingleton(); manager->initialized_) {
        UT_TRACE("UT: Game loaded.");
        manager->OnPostLoadGame();
      }
      break;
    }
  }

  bool Initialize() noexcept
  {
    // Load game objects and references.
    try {
      Game::Load();
    }
    catch (const std::exception& e) {
      UT_PRINT("UT: %s", e.what());
      return false;
    }

    // Get input device manager.
    auto input = RE::BSInputDeviceManager::GetSingleton();
    if (!input) {
      UT_PRINT("UT: Could not get input device manager.");
      return false;
    }

    // Register papyrus functions.
    if (!Game::Papyrus->Register(Script::Register)) {
      UT_PRINT("UT: Could not register papyrus functions.");
      return false;
    }

    // Add input event sink.
    input->AddEventSink<RE::InputEvent*>(this);

    // Get script event source holder.
    // auto sesh = RE::ScriptEventSourceHolder::GetSingleton();
    // if (!sesh) {
    //   UT_PRINT("UT: Could not get script event source holder.");
    //   return false;
    // }

    // Add combat event sink.
    // sesh->AddEventSink<RE::TESCombatEvent>(this);

    // Add hit event sink.
    // sesh->AddEventSink<RE::TESHitEvent>(this);

    initialized_ = true;
    return true;
  }

  static Manager* GetSingleton() noexcept
  {
    static Manager trinity;
    return &trinity;
  }

protected:
  RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*) override
  {
    if (!eventPtr || !*eventPtr) {
      return RE::BSEventNotifyControl::kContinue;
    }

    const auto event = *eventPtr;
    if (event->GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
      return RE::BSEventNotifyControl::kContinue;
    }

    if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
      return RE::BSEventNotifyControl::kContinue;
    }

    const auto button = event->AsButtonEvent();
    if (button->IsRepeating()) {
      return RE::BSEventNotifyControl::kContinue;
    }

    if (RE::UI::GetSingleton()->IsMenuOpen("Console")) {
      return RE::BSEventNotifyControl::kContinue;
    }

    switch (button->idCode) {
    case RE::BSKeyboardDevice::Keys::kC:
      return OnAction();
    case RE::BSKeyboardDevice::Keys::kF:
      return OnEquip();
    }

    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*) override
  {
    if (!event || !event->actor || event->actor->GetFormType() != RE::FormType::ActorCharacter) {
      return RE::BSEventNotifyControl::kContinue;
    }
    return OnCombat(event->actor->As<RE::Actor>(), event->newState.get());
  }

  RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* event, RE::BSTEventSource<RE::TESHitEvent>*) override
  {
    if (!event || !event->target || event->target->GetFormType() != RE::FormType::ActorCharacter) {
      return RE::BSEventNotifyControl::kContinue;
    }
    return OnHit(event->target->As<RE::Actor>(), event->source);
  }

private:
  struct Script {
    static void Add(RE::StaticFunctionTag*, RE::Actor* actor)
    {
      GetSingleton()->Add(actor);
    }

    static void Remove(RE::StaticFunctionTag*, RE::Actor* actor)
    {
      GetSingleton()->Remove(actor);
    }

    static void Update(RE::StaticFunctionTag*)
    {
      GetSingleton()->Update();
    }

    static bool Register(RE::BSScript::IVirtualMachine* vm) noexcept
    {
      // clang-format off
      vm->RegisterFunction("Add",    "UT_Trinity", Add);
      vm->RegisterFunction("Remove", "UT_Trinity", Remove);
      vm->RegisterFunction("Update", "UT_Trinity", Update);
      // clang-format on
      return true;
    }
  };

  void OnPreLoadGame() noexcept
  {
    guard_.reset();
    knight_.reset();
    warlock_.reset();
  }

  void OnPostLoadGame() noexcept
  {
    // Get player process.
    const auto process = Game::Player->GetActorRuntimeData().currentProcess;
    if (!process) {
      UT_PRINT("UT: Could not get player process.");
      return;
    }

    // Get player process middle high.
    const auto middleHigh = process->middleHigh;
    if (!middleHigh) {
      UT_PRINT("UT: Could not get player process middle high.");
      return;
    }

    // Get player commanded actors.
    for (const auto& e : middleHigh->commandedActors) {
      if (const auto actor = e.commandedActor.get()) {
        Add(actor.get());
      }
    }
  }

  RE::BSEventNotifyControl OnAction() noexcept
  {
    // Move actors to player.
    // const auto pos = Game::Player->GetPosition();
    // for (const auto& [id, actors] : actors_) {
    //   for (const auto actor : actors) {
    //     UT_DEBUG("UT: [%s] Move to %.1f %.1f %.1f", Game::GetName(id), pos.x, pos.y, pos.z);
    //     actor->SetPosition(pos, true);
    //   }
    // }

    // Override actor package.
    // static bool added = false;
    // const auto id = Game::Warlock;
    // const auto package = Game::Package::WarlockHeal;
    // if (const auto actor = trinity_[id]) {
    //   if (!added) {
    //     Game::AddPackageOverride(id, actor, package, 2, true, [id, actor, package](RE::BSScript::Variable result) {
    //       UT_TRACE("UT: [%s] PACK %08X (2)", Game::GetName(id), package->GetFormID());
    //       actor->EvaluatePackage(true, true);
    //     });
    //   } else {
    //     Game::RemovePackageOverride(id, actor, package, [id, actor, package](RE::BSScript::Variable result) {
    //       UT_TRACE("UT: [%s] PACK %08X (Removed)", Game::GetName(id), package->GetFormID());
    //       actor->EvaluatePackage(true, true);
    //     });
    //   }
    //   added = !added;
    // }

    std::string info;
    auto ss = std::back_inserter(info);
    if (const auto guard = guard_) {
      std::format_to(ss, " G:{:.1f}", Game::Player->GetPosition().GetDistance(guard->GetPosition()));
    }
    if (const auto knight = knight_) {
      std::format_to(ss, " K:{:.1f}", Game::Player->GetPosition().GetDistance(knight->GetPosition()));
    }
    if (const auto warlock = warlock_) {
      std::format_to(ss, " W:{:.1f}", Game::Player->GetPosition().GetDistance(warlock->GetPosition()));
    }
    if (!info.empty()) {
      UT_PRINT("UT:%s", info.data());
    }

    return RE::BSEventNotifyControl::kStop;
  }

  RE::BSEventNotifyControl OnEquip() noexcept
  {
    // Get user interface.
    const auto ui = RE::UI::GetSingleton();
    if (!ui) {
      UT_PRINT("UT: Could not get user interface.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Check container menu state.
    if (!ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
      UT_TRACE("UT: Container menu not open.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Get container menu target.
    const auto reference = RE::ContainerMenu::GetTargetRefHandle();
    RE::TESObjectREFRPtr target;
    if (!RE::LookupReferenceByHandle(reference, target) || !target) {
      UT_TRACE("UT: Container menu has no target.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Get target base.
    const auto base = target->GetBaseObject();
    if (!base) {
      UT_TRACE("UT: Container menu target has no base.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Check target base form id.
    const auto id = base->GetFormID();
    if (id != Game::Guard && id != Game::Knight && id != Game::Warlock) {
      UT_TRACE("UT: Container menu target is not a trinity member.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Get container menu.
    RE::ContainerMenu* menu = nullptr;
    if (const auto ptr = ui->GetMenu<RE::ContainerMenu>()) {
      menu = ptr.get();
    }
    if (!menu) {
      UT_PRINT("UT: Could not get container menu.");
      return RE::BSEventNotifyControl::kContinue;
    }
    if (menu->GetContainerMode() != RE::ContainerMenu::ContainerMode::kNPCMode) {
      UT_PRINT("UT: Container menu is not in NPC mode.");
      return RE::BSEventNotifyControl::kContinue;
    }
    const auto& data = menu->GetRuntimeData();

    // Get selected item.
    if (!data.itemList) {
      UT_TRACE("UT: Container menu has no item list.");
      return RE::BSEventNotifyControl::kContinue;
    }
    const auto item = data.itemList->GetSelectedItem();
    if (!item) {
      UT_TRACE("UT: Container menu item list has no selected item.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Get selected item info.
    const auto info = item->data.objDesc;
    if (!info) {
      UT_TRACE("UT: Selected item has no object description.");
      return RE::BSEventNotifyControl::kContinue;
    }
    auto object = info->GetObject();
    if (!object) {
      UT_TRACE("UT: Selected item has no object.");
      return RE::BSEventNotifyControl::kContinue;
    }
    if (!info->extraLists || info->extraLists->empty()) {
      UT_TRACE("UT: Selected item has no extra data lists.");
      return RE::BSEventNotifyControl::kContinue;
    }
    auto extra = info->extraLists->front();

    // Get target actor.
    auto actor = target->As<RE::Actor>();
    if (!actor) {
      UT_PRINT("UT: Could not get container menu target actor.");
      return RE::BSEventNotifyControl::kContinue;
    }

    // Try to equip item.
    if (info->IsWorn()) {
      Game::Unequip(id, actor, object, extra);
    } else if (Game::CanEquip(id, object)) {
      Game::Equip(id, actor, object);
    }
    return RE::BSEventNotifyControl::kStop;
  }

  RE::BSEventNotifyControl OnCombat(RE::Actor* actor, RE::ACTOR_COMBAT_STATE state) noexcept
  {
    const auto base = actor->GetActorBase();
    if (!base) {
      return RE::BSEventNotifyControl::kContinue;
    }
    const auto id = base->GetFormID();
    if (id != 0x00000007 && id != Game::Guard && id != Game::Knight && id != Game::Warlock) {
      UT_TRACE("UT: %08X Combat state: %s", id, Game::GetName(state));
      return RE::BSEventNotifyControl::kContinue;
    }
    UT_TRACE("UT: [%s] Combat state: %s", Game::GetName(id), Game::GetName(state));
    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl OnHit(RE::Actor* actor, RE::FormID source) noexcept
  {
    const auto base = actor->GetActorBase();
    if (!base) {
      return RE::BSEventNotifyControl::kContinue;
    }
    const auto id = base->GetFormID();
    if (id != 0x00000007 && id != Game::Guard && id != Game::Knight && id != Game::Warlock) {
      return RE::BSEventNotifyControl::kContinue;
    }
    UT_TRACE("UT: [%s] HITE %08X %4.2f", Game::GetName(id), source, Game::GetHealth(actor));
    return RE::BSEventNotifyControl::kContinue;
  }

  void Add(RE::Actor* actor) noexcept
  {
    if (!actor || actor->IsDead()) {
      return;
    }
    auto trinity = std::make_shared<Trinity>(actor);
    if (trinity->IsGuard()) {
      if (const auto guard = guard_; guard && guard->GetFormID() == trinity->GetFormID()) {
        return;
      }
      guard_ = trinity;
    } else if (trinity->IsKnight()) {
      if (const auto knight = knight_; knight && knight->GetFormID() == trinity->GetFormID()) {
        return;
      }
      knight_ = trinity;
    } else if (trinity->IsWarlock()) {
      if (const auto warlock = warlock_; warlock && warlock->GetFormID() == trinity->GetFormID()) {
        return;
      }
      warlock_ = trinity;
    } else {
      return;
    }
    trinity->Initialize();
    UT_TRACE("UT: [%s] %08X Added to actors list.", Game::GetName(trinity->GetClass()), actor->GetFormID());
  }

  void Remove(RE::Actor* actor) noexcept
  {
    if (!actor) {
      return;
    }
    const auto base = actor->GetActorBase();
    if (!base) {
      return;
    }
    const auto id = base->GetFormID();
    if (id == Game::Guard) {
      if (guard_ && guard_->GetFormID() == actor->GetFormID()) {
        guard_.reset();
        UT_TRACE("UT: [%s] %08X Removed from actors list.", Game::GetName(id), actor->GetFormID());
      }
    } else if (id == Game::Knight) {
      if (knight_ && knight_->GetFormID() == actor->GetFormID()) {
        knight_.reset();
        UT_TRACE("UT: [%s] %08X Removed from actors list.", Game::GetName(id), actor->GetFormID());
      }
    } else if (id == Game::Warlock) {
      if (warlock_ && warlock_->GetFormID() == actor->GetFormID()) {
        warlock_.reset();
        UT_TRACE("UT: [%s] %08X Removed from actors list.", Game::GetName(id), actor->GetFormID());
      }
    }
  }

  void Update() noexcept
  {
    if (const auto warlock = warlock_) {
      warlock->SetCombatPackage(GetCombatPackage(warlock->GetActor()));
    }
  }

  RE::TESPackage* GetCombatPackage(RE::Actor* warlock) noexcept
  {
    if (!warlock || warlock->IsDead()) {
      return nullptr;
    }
    auto playerMin = 1.0f;
    auto warlockMin = 1.0f;
    auto knightMin = 1.0f;
    auto guardMin = 1.0f;
    if (Game::Player->IsInCombat()) {
      playerMin = 0.9f;
      warlockMin = 0.9f;
      knightMin = 0.75f;
      guardMin = 0.6f;
    }
    if (!Game::Player->IsDead() && Game::GetHealth(Game::Player) < playerMin) {
      return Game::Heal;
    }
    if (Game::GetHealth(warlock) < warlockMin) {
      return Game::HealSelf;
    }
    const auto pos = warlock->GetPosition();
    if (const auto knight = knight_) {
      const auto health = Game::GetHealth(knight->GetActor());
      if (!knight->IsDead() && health > 0.1f && health < knightMin && pos.GetDistance(knight->GetPosition()) < 900.0f) {
        return Game::HealKnight;
      }
    }
    if (const auto guard = guard_) {
      const auto health = Game::GetHealth(guard->GetActor());
      if (!guard->IsDead() && health > 0.1f && health < guardMin && pos.GetDistance(guard->GetPosition()) < 900.0f) {
        return Game::HealGuard;
      }
    }
    return nullptr;
  }

  bool initialized_{ false };
  std::shared_ptr<Trinity> guard_;
  std::shared_ptr<Trinity> knight_;
  std::shared_ptr<Trinity> warlock_;
};

}  // namespace UT

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
  SKSE::Init(skse);
  if (!SKSE::GetMessagingInterface()->RegisterListener(UT::Manager::Listener)) {
    return false;
  }
  return true;
}
