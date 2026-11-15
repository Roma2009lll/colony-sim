#pragma once

#include <algorithm>

namespace colony::agents {

struct Needs {
  float hunger{0.0f};
  float health{100.0f};

  static constexpr float kill_hunger = 100.0f;
  static constexpr float k_starving_threshold = 70.0f;
  static constexpr float k_default_hunger_rate = 1.0f;

  [[nodiscard]] constexpr bool is_starving() const noexcept {
    return hunger >= k_starving_threshold;
  }

  [[nodiscard]] constexpr bool is_hungry() const noexcept {
    return is_starving();
  }

  [[nodiscard]] constexpr bool is_dead() const noexcept {
    return health <= 0.0f || hunger >= kill_hunger;
  }

  constexpr void apply_hunger(float amount) noexcept {
    if (is_dead() || amount <= 0.0f) {
      return;
    }
    hunger = std::clamp(hunger + amount, 0.0f, kill_hunger);
    if (hunger >= kill_hunger) {
      health = 0.0f;
    }
  }

  constexpr void tick(float rate = k_default_hunger_rate) noexcept {
    apply_hunger(rate);
  }

  constexpr void eat(float nutrition) noexcept {
    if (is_dead() || nutrition <= 0.0f) {
      return;
    }
    hunger = std::max(0.0f, hunger - nutrition);
  }
};

} // namespace colony::agents