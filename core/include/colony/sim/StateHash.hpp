#pragma once

#include <cstdint>
#include <span>

#include <colony/agents/Agent.hpp>
#include <colony/tasks/PriorityPolicy.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/world/World.hpp>

namespace colony::sim {
using StateHash = std::uint64_t;

[[nodiscard]] StateHash
compute_state_hash(uint64_t tick, const world::World &world,
                   std::span<const agents::Agent> agents,
                   const tasks::TaskBoard &task_board,
                   const tasks::PriorityPolicy &priority_policy);
} // namespace colony::sim