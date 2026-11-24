#include <colony/sim/Resolver.hpp>

#include <colony/pathfinding/BFS.hpp>
#include <colony/pathfinding/GridGraph.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <vector>

namespace colony::sim {
namespace {

[[nodiscard]] std::uint32_t manhattan_distance(util::Cell a,
                                               util::Cell b) noexcept {
  const auto dx = std::abs(static_cast<int>(a.x) - static_cast<int>(b.x));
  const auto dy = std::abs(static_cast<int>(a.y) - static_cast<int>(b.y));
  return static_cast<std::uint32_t>(dx + dy);
}

[[nodiscard]] std::vector<std::size_t>
build_execution_order(std::size_t count,
                      std::span<const agents::Agent> agents) {
  std::vector<std::size_t> order;
  order.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    if (agents[i].is_alive()) {
      order.push_back(i);
    }
  }

  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return agents[a].id() < agents[b].id();
  });

  return order;
}

} // namespace

std::optional<util::Cell> find_next_step(const world::World &world,
                                         util::Cell start, util::Cell end) {
  if (!world.in_bounds(start) || !world.in_bounds(end) ||
      !world[start].is_walkable() || start == end) {
    return std::nullopt;
  }

  thread_local pathfinding::BfsPathFinder bfs_finder{};
  pathfinding::GridGraph finder{
      world.grid(), [](const world::Tile &t) { return t.is_walkable(); }};

  const pathfinding::NodeId start_node = finder.to_node(start);

  if (world[end].is_walkable()) {
    const auto view =
        bfs_finder.find_path(finder, start_node, finder.to_node(end));
    return view.path_.size() >= 2 ? std::optional{finder.to_cell(view.path_[1])}
                                  : std::nullopt;
  }

  if (manhattan_distance(start, end) == 1) {
    return std::nullopt;
  }

  static constexpr std::array<util::Cell, 4> k_dirs{
      {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

  pathfinding::NodeId best_node = pathfinding::k_invalid_node;
  pathfinding::Cost min_cost = std::numeric_limits<pathfinding::Cost>::max();

  for (const auto &dir : k_dirs) {
    const util::Cell next = end + dir;
    if (!world.in_bounds(next) || !world[next].is_walkable()) {
      continue;
    }

    const auto view =
        bfs_finder.find_path(finder, start_node, finder.to_node(next));
    if (view.path_.size() >= 2 && view.total_cost_ < min_cost) {
      min_cost = view.total_cost_;
      best_node = view.path_[1];
    }
  }

  return best_node != pathfinding::k_invalid_node
             ? std::optional{finder.to_cell(best_node)}
             : std::nullopt;
}

void resolve(std::span<const ai::Decision> decisions,
             std::span<agents::Agent> agents, world::World &world,
             tasks::TaskBoard &task_board, metrics::Stats &stats,
             ResolveConfig config) {
  const std::size_t count = std::min(agents.size(), decisions.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (agents[i].is_dead() && agents[i].has_claimed_task()) {
      task_board.release_all_by_agent(agents[i].agent_id());
      agents[i].clear_claimed_task();
    }
  }
  const std::vector<std::size_t> order = build_execution_order(count, agents);
  for (const std::size_t idx : order) {
    agents::Agent &agent = agents[idx];
    const ai::Decision &decision = decisions[idx];
    switch (decision.action) {
    case ai::ActionType::Idle: {
      break;
    }
    case ai::ActionType::MoveStep: {
      if (const auto next =
              find_next_step(world, agent.position(), decision.target_cell);
          next.has_value()) {
        agent.set_position(*next);
      }
      break;
    }
    case ai::ActionType::ClaimTask: {
      if (agent.has_claimed_task()) {
        break;
      }
      const auto res = task_board.claim(decision.task_id, agent.agent_id());
      if (res.ok()) {
        agent.set_claimed_task(res.task_id);
      }
      break;
    }
    case ai::ActionType::Gather: {
      if (!world.in_bounds(decision.target_cell) ||
          manhattan_distance(agent.position(), decision.target_cell) > 1) {
        break;
      }
      world::Tile &tile = world[decision.target_cell];
      if (!tile.has_resource() || tile.resource_amount == 0) {
        break;
      }
      const auto &inv = agent.get_inventory();
      if (agent.has_resource() && inv.resource != tile.resource) {
        break;
      }
      const std::uint32_t carried =
          agent.has_resource() ? inv.resource_amount : 0;
      if (carried >= config.max_inventory) {
        break;
      }
      const std::uint32_t space = config.max_inventory - carried;
      const std::uint32_t taken = std::min(space, tile.resource_amount);
      agent.set_inventory(tile.resource, carried + taken);
      tile.resource_amount -= taken;
      if (tile.resource_amount == 0) {
        tile.reset();
      }
      break;
    }
    case ai::ActionType::Deliver: {
      if (!world.in_bounds(decision.target_cell) ||
          manhattan_distance(agent.position(), decision.target_cell) > 1) {
        break;
      }
      if (world[decision.target_cell].terrain != world::TerrainType::Storage ||
          !agent.has_resource()) {
        break;
      }
      const auto inv = agent.get_inventory();
      if (inv.resource == world::ResourceType::Food) {
        world.add_stored_food(inv.resource_amount);
        stats.food_collected += inv.resource_amount;
      } else if (inv.resource == world::ResourceType::Wood) {
        world.add_stored_wood(inv.resource_amount);
        stats.wood_collected += inv.resource_amount;
      }
      agent.clear_inventory();
      if (agent.has_claimed_task()) {
        const auto res =
            task_board.complete(agent.claimed_task_id(), agent.agent_id());
        if (res.ok()) {
          ++stats.tasks_completed;
        }
        agent.clear_claimed_task();
      }
      break;
    }

    case ai::ActionType::Eat: {
      bool ate = false;
      if (agent.has_resource() &&
          agent.get_inventory().resource == world::ResourceType::Food) {
        const std::uint32_t left = agent.get_inventory().resource_amount - 1;
        if (left == 0) {
          agent.clear_inventory();
        } else {
          agent.set_inventory(world::ResourceType::Food, left);
        }
        ate = true;
      } else if (world.in_bounds(decision.target_cell) &&
                 manhattan_distance(agent.position(), decision.target_cell) <=
                     1) {
        world::Tile &tile = world[decision.target_cell];
        if (tile.has_resource() && tile.resource == world::ResourceType::Food &&
            tile.resource_amount > 0) {
          --tile.resource_amount;
          if (tile.resource_amount == 0) {
            tile.reset();
          }
          ate = true;
        }
      }
      if (ate) {
        agent.needs().eat(config.food_nutrition);
      }
      break;
    }
    }
  }
  task_board.sync_with_world(world);
}

} // namespace colony::sim