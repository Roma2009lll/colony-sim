#pragma once

#include <colony/world/Tile.hpp>

#include <algorithm>
#include <cstdint>

namespace colony::tasks {

class PriorityPolicy {
private:
  std::uint32_t food_weight_{k_default_food_weight};

public:
  static constexpr std::uint32_t k_min_weight = 0;
  static constexpr std::uint32_t k_max_weight = 100;
  static constexpr std::uint32_t k_default_food_weight = 50;

  constexpr PriorityPolicy() noexcept = default;

  explicit constexpr PriorityPolicy(std::uint32_t food_weight) noexcept
      : food_weight_(std::clamp(food_weight, k_min_weight, k_max_weight)) {}

  [[nodiscard]] constexpr std::uint32_t food_weight() const noexcept {
    return food_weight_;
  }

  [[nodiscard]] constexpr std::uint32_t wood_weight() const noexcept {
    return k_max_weight - food_weight_;
  }

  constexpr void set_food_weight(std::uint32_t food_weight) noexcept {
    food_weight_ = std::clamp(food_weight, k_min_weight, k_max_weight);
  }

  [[nodiscard]] constexpr std::uint32_t
  weight_for(colony::world::ResourceType resource) const noexcept {
    switch (resource) {
    case colony::world::ResourceType::Food:
      return food_weight();
    case colony::world::ResourceType::Wood:
      return wood_weight();
    default:
      return 0;
    }
  }

  constexpr bool operator==(const PriorityPolicy &) const noexcept = default;
};

} // namespace colony::tasks