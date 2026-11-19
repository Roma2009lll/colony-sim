#include <colony/ai/Decision.hpp>

namespace colony::ai {
namespace {
[[nodiscard]] std::uint32_t manhattan_distance(util::Cell a,
                                               util::Cell b) noexcept {
  const auto dx = std::abs(static_cast<int>(a.x) - static_cast<int>(b.x));
  const auto dy = std::abs(static_cast<int>(a.y) - static_cast<int>(b.y));
  return static_cast<std::uint32_t>(dx + dy);
}

[[nodiscard]] bool in_reach(util::Cell agent_pos, util::Cell target) noexcept {
  return manhattan_distance(agent_pos, target) <= 1;
}

[[nodiscard]] std::optional<util::Cell>
find_nearest_food(util::Cell from, const world::World &world) noexcept {
  if (!world.in_bounds(from)) {
    return std::nullopt;
  }
  std::optional<util::Cell> best_cell = std::nullopt;
  std::uint32_t best_dist = std::numeric_limits<std::uint32_t>::max();
  for (std::size_t y = 0; y < world.height(); ++y) {
    for (std::size_t x = 0; x < world.width(); ++x) {
      util::Cell cell{static_cast<decltype(util::Cell::x)>(x),
                      static_cast<decltype(util::Cell::y)>(y)};
      const auto &tile = world[cell];
      const bool has_bush_food =
          tile.resource == world::ResourceType::Food && tile.has_resource();
      const bool has_stored_food =
          tile.terrain == world::TerrainType::Storage &&
          world.stored_food() > 0;
      if (has_bush_food || has_stored_food) {
        const std::uint32_t dist = manhattan_distance(from, cell);
        if (dist < best_dist) {
          best_dist = dist;
          best_cell = cell;
        }
      }
    }
  }
  return best_cell;
}

[[nodiscard]] std::optional<util::Cell>
find_nearest_storage(util::Cell from, const world::World &world) noexcept {
  if (!world.in_bounds(from)) {
    return std::nullopt;
  }
  std::optional<util::Cell> best_cell = std::nullopt;
  std::uint32_t best_dist = std::numeric_limits<std::uint32_t>::max();
  for (std::size_t y = 0; y < world.height(); ++y) {
    for (std::size_t x = 0; x < world.width(); ++x) {
      util::Cell cell{static_cast<decltype(util::Cell::x)>(x),
                      static_cast<decltype(util::Cell::y)>(y)};
      if (world[cell].terrain == world::TerrainType::Storage) {
        const std::uint32_t dist = manhattan_distance(from, cell);
        if (dist < best_dist) {
          best_dist = dist;
          best_cell = cell;
        }
      }
    }
  }
  return best_cell;
}

[[nodiscard]] const tasks::Task *
find_claimed_task(tasks::AgentId id, const tasks::TaskBoard &task_board) {
  for (const auto &it : task_board.task_board()) {
    if (it.assignee == id && it.state == tasks::TaskState::Claimed) {
      return &it;
    }
  }
  return nullptr;
}

const tasks::Task *find_nearest_open_task(
    util::Cell from, const tasks::TaskBoard &board,
    tasks::TaskPriority priority,
    std::optional<world::ResourceType> resource_filter) noexcept {
  const tasks::Task *best_task = nullptr;
  std::uint32_t best_dist = std::numeric_limits<std::uint32_t>::max();
  for (const auto &task : board.task_board()) {
    if (task.state != tasks::TaskState::Open) {
      continue;
    }
    if (task.priority != priority) {
      continue;
    }
    if (resource_filter.has_value() && task.resource != *resource_filter) {
      continue;
    }
    const std::uint32_t dist = manhattan_distance(from, task.target_cell);
    if (dist < best_dist || (dist == best_dist && best_task != nullptr &&
                             task.id.value() < best_task->id.value())) {
      best_dist = dist;
      best_task = &task;
    }
  }
  return best_task;
}

} // namespace
Decision decide(const agents::Agent &agent, const world::World &world,
                const tasks::TaskBoard &task_board, PriorityPolicy policy,
                util::Rng &rng) {
  if (agent.is_dead()) {
    return Decision{};
  }
  if (agent.needs().is_hungry()) {
    if (const auto food_cell = find_nearest_food(agent.position(), world);
        food_cell.has_value()) {
      if (in_reach(agent.position(), *food_cell)) {
        return Decision{ActionType::Eat, *food_cell};
      }
      return Decision{ActionType::MoveStep, *food_cell};
    }
  }
  if (agent.has_resource()) {
    if (const auto storage_cell = find_nearest_storage(agent.position(), world);
        storage_cell.has_value()) {
      if (in_reach(agent.position(), *storage_cell)) {
        return Decision{ActionType::Deliver, *storage_cell};
      }
      return Decision{ActionType::MoveStep, *storage_cell};
    }
  }
  if (const tasks::Task *claimed =
          find_claimed_task(agent.agent_id(), task_board);
      claimed != nullptr) {
    if (in_reach(agent.position(), claimed->target_cell)) {
      return Decision{ActionType::Gather, claimed->target_cell, claimed->id};
    }
    return Decision{ActionType::MoveStep, claimed->target_cell, claimed->id};
  }
  const tasks::Task *chosen_task =
      find_nearest_open_task(agent.position(), task_board,
                             tasks::TaskPriority::Directive, std::nullopt);
  if (chosen_task == nullptr) {
    const bool prefer_food = rng.range(0, 99) < policy.food_weight_;
    const auto primary_res =
        prefer_food ? world::ResourceType::Food : world::ResourceType::Wood;
    chosen_task = find_nearest_open_task(
        agent.position(), task_board, tasks::TaskPriority::Normal, primary_res);
    if (chosen_task == nullptr) {
      chosen_task =
          find_nearest_open_task(agent.position(), task_board,
                                 tasks::TaskPriority::Normal, std::nullopt);
    }
  }
  if (chosen_task != nullptr) {
    return Decision{ActionType::ClaimTask, chosen_task->target_cell,
                    chosen_task->id};
  }
  return Decision{};
}
} // namespace colony::ai