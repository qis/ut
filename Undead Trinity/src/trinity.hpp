#pragma once
#include <game.hpp>

namespace UT {

class Trinity : public std::enable_shared_from_this<Trinity> {
public:
  Trinity(RE::Actor* actor);
  Trinity(Trinity&& other) = delete;
  Trinity(const Trinity& other) = delete;
  Trinity& operator=(Trinity&& other) = delete;
  Trinity& operator=(const Trinity& other) = delete;
  ~Trinity();

  void Initialize() noexcept;
  void SetCombatPackage(RE::TESPackage* package) noexcept;

  bool IsGuard() const noexcept
  {
    return class_ == Game::Guard;
  }

  bool IsKnight() const noexcept
  {
    return class_ == Game::Knight;
  }

  bool IsWarlock() const noexcept
  {
    return class_ == Game::Warlock;
  }

  bool IsTrinity() const noexcept
  {
    return IsGuard() || IsKnight() || IsWarlock();
  }

  constexpr RE::Actor* GetActor() const noexcept
  {
    return actor_;
  }

  constexpr RE::FormID GetClass() const noexcept
  {
    return class_;
  }

  auto GetFormID() const noexcept
  {
    return actor_->GetFormID();
  }

  auto GetPosition() const noexcept
  {
    return actor_->GetPosition();
  }

  auto IsDead() const noexcept
  {
    return actor_->IsDead();
  }

private:
  static RE::FormID GetTrinityClass(RE::Actor* actor) noexcept;
  void ClearPackages(std::function<void()> callback = {}) noexcept;

  RE::Actor* actor_;
  RE::FormID class_;
  bool initialized_{ false };
  RE::TESPackage* package_{ nullptr };
};

}  // namespace UT
