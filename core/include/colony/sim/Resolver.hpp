#pragma once

#include <colony/agents/Agent.hpp>
#include <colony/ai/Decision.hpp>
#include <colony/metrics/Stats.hpp>
#include <colony/pathfinding/BFS.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/util/Grid.hpp>
#include <colony/world/World.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace colony::sim {
struct ResolveConfig {
  std::uint32_t max_inventory{10};
  float food_nutrition{50.0f};
};

[[nodiscard]] std::optional<util::Cell>
find_next_step(const world::World &world, util::Cell start, util::Cell end);

void resolve(std::span<const ai::Decision> decisions,
             std::span<agents::Agent> agents, world::World &world,
             tasks::TaskBoard &task_board, metrics::Stats &stats);
} // namespace colony::sim
