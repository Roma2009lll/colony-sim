#pragma once

#include <colony/agents/Agent.hpp>
#include <colony/ai/Decision.hpp>
#include <colony/metrics/Stats.hpp>
#include <colony/sim/CommandLog.hpp>
#include <colony/sim/Resolver.hpp>
#include <colony/sim/StateHash.hpp>
#include <colony/tasks/PriorityPolicy.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/util/Rng.hpp>
#include <colony/world/World.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace colony::sim {

struct SimulationConfig {
  std::size_t max_agents{20};
  float hunger_rate{agents::Needs::k_default_hunger_rate};
  double resource_regrow_chance{0.02};
  std::uint32_t regrow_amount{5};
  std::uint32_t birth_food_cost{20};
  std::uint32_t birth_wood_cost{10};
  ResolveConfig resolve_config{};
};

class Simulation {
private:
  world::World world_;
  tasks::TaskBoard task_board_{};
  std::vector<agents::Agent> agents_{};
  tasks::PriorityPolicy priority_policy_{};
  CommandLog command_log_{};
  std::vector<CommandReceipt> receipts_{};
  metrics::Stats stats_{};
  SimulationConfig config_{};
  std::uint64_t seed_{0};
  std::uint64_t tick_{0};
  std::uint32_t next_agent_id_{1};
  util::Rng world_rng_{};

  void apply_tick_commands();

  void update_world_and_needs();

  [[nodiscard]] std::vector<ai::Decision> plan_intents() const;

  void commit_tick();

public:
  Simulation(world::World world, std::vector<agents::Agent> initial_agents,
             std::uint64_t seed = 0, CommandLog command_log = {},
             SimulationConfig config = {});

  void step();

  void run(std::uint64_t max_ticks);

  [[nodiscard]] bool is_finished(std::uint64_t max_ticks) const noexcept {
    return tick_ >= max_ticks || agents_.empty();
  }

  [[nodiscard]] std::uint64_t tick() const noexcept { return tick_; }

  [[nodiscard]] std::uint64_t seed() const noexcept { return seed_; }

  [[nodiscard]] const world::World &world() const noexcept { return world_; }

  [[nodiscard]] const tasks::TaskBoard &task_board() const noexcept {
    return task_board_;
  }
  [[nodiscard]] std::span<const agents::Agent> agents() const noexcept {
    return agents_;
  }
  [[nodiscard]] const tasks::PriorityPolicy &priority_policy() const noexcept {
    return priority_policy_;
  }
  [[nodiscard]] std::span<const CommandReceipt> receipts() const noexcept {
    return receipts_;
  }
  [[nodiscard]] const metrics::Stats &stats() const noexcept { return stats_; }

  [[nodiscard]] StateHash state_hash() const {
    return compute_state_hash(tick_, world_, agents_, task_board_,
                              priority_policy_);
  }
};

} // namespace colony::sim