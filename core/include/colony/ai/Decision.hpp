#pragma once

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>

#include <colony/agents/Agent.hpp>
#include <colony/tasks/Task.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/util/Grid.hpp>
#include <colony/util/Rng.hpp>
#include <colony/world/World.hpp>

namespace colony::ai {
struct PriorityPolicy {
  std::uint8_t food_weight_{50};
};
enum class ActionType : std::uint8_t {
  Idle,
  MoveStep,
  Eat,
  Gather,
  Deliver,
  ClaimTask
};
struct Decision {
  ActionType action{ActionType::Idle};
  util::Cell target_cell{};
  tasks::TaskId task_id{tasks::TaskId::invalid()};
};

[[nodiscard]] Decision decide(const agents::Agent &agent,
                              const world::World &world,
                              const tasks::TaskBoard &task_board,
                              PriorityPolicy policy, util::Rng &rng);
} // namespace colony::ai