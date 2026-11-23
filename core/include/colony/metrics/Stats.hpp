#pragma once

#include <colony/sim/StateHash.hpp>

#include <cstdint>

namespace colony::metrics {

struct Stats {
  std::uint64_t food_collected{0};
  std::uint64_t wood_collected{0};
  std::uint64_t tasks_completed{0};
  std::uint64_t colonists_born{0};
  std::uint64_t colonists_died{0};
  std::uint64_t directives_accepted{0};
  std::uint64_t directives_rejected{0};
  sim::StateHash final_state_hash{0};

  constexpr bool operator==(const Stats &) const noexcept = default;
};

} // namespace colony::metrics