#include "trinity.hpp"

namespace UT {
namespace {

constexpr auto RP = Game::RemovePackageOverride;

}  // namespace

Trinity::Trinity(RE::Actor* actor) :
  actor_(actor),
  class_(GetTrinityClass(actor))
{}

Trinity::~Trinity()
{
  if (initialized_) {
    ClearPackages();
  }
}

void Trinity::Initialize() noexcept
{
  if (initialized_ || !IsTrinity()) {
    return;
  }
  initialized_ = true;
  Game::Initialize(class_, actor_);
  ClearPackages([this, self = shared_from_this()]() { actor_->EvaluatePackage(true, false); });
}

void Trinity::SetCombatPackage(RE::TESPackage* package) noexcept
{
  if (package == package_) {
    return;
  }
  const auto self = shared_from_this();
  if (package_) {
    Game::RemovePackageOverride(class_, actor_, package_, [this, self, package](RE::BSScript::Variable result) {
      if (package) {
        Game::AddPackageOverride(class_, actor_, package, 2, true, [this, self, package](RE::BSScript::Variable result) {
          UT_DEBUG("UT: [%s] PACK %s", Game::GetName(class_), Game::GetName(package));
          actor_->EvaluatePackage(true, false);
        });
      } else {
        UT_DEBUG("UT: [%s] PACK Follow", Game::GetName(class_));
        actor_->EvaluatePackage(true, false);
      }
    });
  } else {
    Game::AddPackageOverride(class_, actor_, package, 2, true, [this, self, package](RE::BSScript::Variable result) {
      UT_DEBUG("UT: [%s] PACK %s", Game::GetName(class_), Game::GetName(package));
      actor_->EvaluatePackage(true, false);
    });
  }
  package_ = package;
}

RE::FormID Trinity::GetTrinityClass(RE::Actor* actor) noexcept
{
  if (actor && !actor->IsDead()) {
    if (const auto base = actor->GetActorBase()) {
      return base->GetFormID();
    }
  }
  return 0;
}

void Trinity::ClearPackages(std::function<void()> callback) noexcept
{
  const auto id = class_;
  if (id != Game::Warlock) {
    return;
  }
  const auto actor = actor_;
  RP(id, actor, Game::HealSelf, [id, actor, callback = std::move(callback)](RE::BSScript::Variable result) mutable {
    RP(id, actor, Game::HealKnight, [id, actor, callback = std::move(callback)](RE::BSScript::Variable result) mutable {
      RP(id, actor, Game::HealGuard, [id, actor, callback = std::move(callback)](RE::BSScript::Variable result) mutable {
        RP(id, actor, Game::Heal, [id, actor, callback = std::move(callback)](RE::BSScript::Variable result) {
          if (callback) {
            callback();
          }
        });
      });
    });
  });
}

}  // namespace UT
