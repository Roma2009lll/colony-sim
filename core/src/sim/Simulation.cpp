#include <colony/sim/Simulation.hpp>

#include <algorithm>
#include <cassert>
#include <utility>
#include <variant>

namespace colony::sim {
namespace {
[[nodiscard]] constexpr CommandRejectReason
to_reject_reason(tasks::TaskError error) noexcept {
  switch (error) {
  case tasks::TaskError::None:
    return CommandRejectReason::None;
  case tasks::TaskError::OutOfBounds:
    return CommandRejectReason::OutOfBounds;
  case tasks::TaskError::NoResource:
    return CommandRejectReason::NoResource;
  case tasks::TaskError::DuplicateDirective:
    return CommandRejectReason::DuplicateDirective;
  case tasks::TaskError::TaskNotFound:
    return CommandRejectReason::TaskNotFound;
  case tasks::TaskError::AlreadyTerminal:
    return CommandRejectReason::AlreadyTerminal;
  default:
    return CommandRejectReason::TaskNotFound;
  }
}
} // namespace

Simulation::Simulation(world::World world,
                       std::vector<agents::Agent> initial_agents,
                       std::uint64_t seed, CommandLog command_log,
                       SimulationConfig config)
    : world_(std::move(world)), agents_(std::move(initial_agents)),
      command_log_(std::move(command_log)), config_(config), seed_(seed),
      world_rng_(seed) {
  std::uint32_t max_id = 0;
  std::ranges::sort(agents_, {}, &agents::Agent::id);
  for (const auto &agent : agents_) {
    max_id = std::max(max_id, agent.id());
  }
  next_agent_id_ = max_id + 1;
  task_board_.sync_with_world(world_);
  stats_.final_state_hash = state_hash();
}

void Simulation::apply_tick_commands() {
  std::span<const CommandEntry> comands_in_tick =
      command_log_.commands_for_tick(tick_);
  for (const auto &com : comands_in_tick) {
    CommandReceipt receipt{.tick_ = tick_, .command_ = com.command_};
    std::visit(
        [&receipt, this](const auto &cmd) {
          using T = std::decay_t<decltype(cmd)>;
          if constexpr (std::is_same_v<T, MarkDirective>) {
            auto res = task_board_.create_directive(world_, cmd.cell_);
            if (res.ok()) {
              receipt.accepted_ = true;
              receipt.reason_ = CommandRejectReason::None;
              stats_.directives_accepted++;
            } else {
              receipt.accepted_ = false;
              receipt.reason_ = to_reject_reason(res.error);
              stats_.directives_rejected++;
            }
          } else if constexpr (std::is_same_v<T, CancelTask>) {
            auto res = task_board_.cancel(cmd.task_id_);
            if (res.ok()) {
              receipt.accepted_ = true;
              receipt.reason_ = CommandRejectReason::None;
            } else {
              receipt.accepted_ = false;
              receipt.reason_ = to_reject_reason(res.error);
            }
          } else if constexpr (std::is_same_v<T, SetBalance>) {
            if (cmd.food_weight_ <= tasks::PriorityPolicy::k_max_weight) {
              priority_policy_.set_food_weight(cmd.food_weight_);
              receipt.accepted_ = true;
              receipt.reason_ = CommandRejectReason::None;
            } else {
              receipt.accepted_ = false;
              receipt.reason_ = CommandRejectReason::InvalidBalance;
            }
          }
        },
        com.command_);
    receipts_.push_back(std::move(receipt));
  }
}

void Simulation::update_world_and_needs() {
  for (auto &agent : agents_) {
    if (agent.is_alive()) {
      agent.needs().tick(config_.hunger_rate);
    }
  }
  for (std::size_t y = 0; y < world_.height(); y++) {
    for (std::size_t x = 0; x < world_.width(); x++) {
      util::Cell current_cell{static_cast<decltype(util::Cell::x)>(x),
                              static_cast<decltype(util::Cell::y)>(y)};
      if (world_[current_cell].terrain == world::TerrainType::Ground &&
          !world_[current_cell].has_resource() &&
          current_cell != world_.start_position()) {
        if (world_rng_.chance(config_.resource_regrow_chance)) {
          world_[current_cell].resource = world_rng_.chance(0.5)
                                              ? world::ResourceType::Food
                                              : world::ResourceType::Wood;
          world_[current_cell].resource_amount = config_.regrow_amount;
        }
      }
    }
  }
  task_board_.sync_with_world(world_);
}
std::vector<ai::Decision> Simulation::plan_intents() const {
  std::vector<ai::Decision> intents(agents_.size());
  for (std::size_t i = 0; i < agents_.size(); i++) {
    if (agents_[i].is_alive()) {
      util::Rng rng = util::Rng::for_agent(seed_, tick_, agents_[i].id());
      intents[i] =
          ai::decide(agents_[i], world_, task_board_, priority_policy_, rng);
    }
  }
  return intents;
}

void Simulation::commit_tick() {
  for (auto &agent : agents_) {
    if (agent.is_dead()) {
      task_board_.release_all_by_agent(agent.agent_id());
      agent.clear_claimed_task();
      stats_.colonists_died++;
    }
  }
  std::erase_if(agents_,
                [](const agents::Agent &agent) { return agent.is_dead(); });
  if (agents_.size() < config_.max_agents &&
      world_.stored_food() >= config_.birth_food_cost &&
      world_.stored_wood() >= config_.birth_wood_cost) {
    world_.consume_stored_food(config_.birth_food_cost);
    world_.consume_stored_wood(config_.birth_wood_cost);
    agents_.emplace_back(next_agent_id_++, world_.start_position());
    stats_.colonists_born++;
  }
  tick_++;
  stats_.final_state_hash = state_hash();
  assert(world_.check_invariants());
}

void Simulation::step() {
  apply_tick_commands();
  update_world_and_needs();
  std::vector<ai::Decision> intends = plan_intents();
  resolve(intends, agents_, world_, task_board_, stats_,config_.resolve_config);
  commit_tick();
}

void Simulation::run(std::uint64_t max_ticks) {
  while (!is_finished(max_ticks)) {
    step();
  }
}

} // namespace colony::sim