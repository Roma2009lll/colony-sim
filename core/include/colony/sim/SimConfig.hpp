#pragma once

#include <cstdint>

namespace colony::sim {

struct SimConfig {
  std::uint32_t initial_agents_{3};
  std::uint32_t max_agents_{20};
  std::uint32_t birth_food_cost_{25};
  std::uint32_t birth_wood_cost_{15};

  std::uint32_t agent_carry_capacity_{5};
  float hunger_per_tick_{1.0f};
  float food_nutrition_{45.0f};

  std::uint32_t gather_amount_per_tick_{1};
  std::uint32_t food_regrowth_interval_{12};
  std::uint32_t food_regrowth_amount_{1};
  std::uint32_t max_tile_food_{10};

  std::uint32_t target_ticks_{1000};
  std::uint32_t target_population_{6};
  std::uint32_t target_stored_food_{40};
  std::uint32_t target_stored_wood_{40};

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return max_agents_ > 0 && initial_agents_ <= max_agents_ &&
           agent_carry_capacity_ > 0 && gather_amount_per_tick_ > 0 &&
           max_tile_food_ > 0 && hunger_per_tick_ >= 0.0f &&
           food_nutrition_ > 0.0f;
  }

  constexpr bool operator==(const SimConfig &) const noexcept = default;
};

} // namespace colony::sim