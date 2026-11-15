#pragma once

#include <cstdint>

#include <colony/agents/Needs.hpp>
#include <colony/tasks/Task.hpp>
#include <colony/util/Grid.hpp>
#include <colony/world/Tile.hpp>

namespace colony::agents {

enum class AgentIntent : std::uint8_t {
  Unknown,
  Idle,
  MoveToResource,
  GatherResource,
  MoveToStorage,
  MoveToFood,
  Eat
};

struct Inventory {
  std::uint32_t resource_amount{0};
  colony::world::ResourceType resource{colony::world::ResourceType::None};
};

class Agent {
private:
  std::uint32_t id_;
  colony::util::Cell position_;
  colony::agents::Needs needs_;
  AgentIntent intent_{AgentIntent::Unknown};
  colony::util::Cell target_cell_;
  Inventory inventory_;
  colony::tasks::TaskId claimed_task_id_{colony::tasks::TaskId::invalid()};

public:
  explicit constexpr Agent(std::uint32_t id,
                           colony::util::Cell position) noexcept
      : id_(id), position_(position) {}

  [[nodiscard]] constexpr std::uint32_t id() const noexcept { return id_; }

  [[nodiscard]] constexpr colony::tasks::AgentId agent_id() const noexcept {
    return colony::tasks::AgentId{id_};
  }

  [[nodiscard]] constexpr colony::util::Cell position() const noexcept {
    return position_;
  }
  constexpr void set_position(colony::util::Cell pos) noexcept {
    position_ = pos;
  }

  [[nodiscard]] constexpr colony::util::Cell get_target_cell() const noexcept {
    return target_cell_;
  }
  constexpr void set_target_cell(colony::util::Cell cell) noexcept {
    target_cell_ = cell;
  }

  [[nodiscard]] constexpr const Needs &needs() const noexcept { return needs_; }
  [[nodiscard]] constexpr Needs &needs() noexcept { return needs_; }

  [[nodiscard]] constexpr bool is_alive() const noexcept {
    return !needs_.is_dead();
  }
  [[nodiscard]] constexpr bool is_dead() const noexcept {
    return needs_.is_dead();
  }

  [[nodiscard]] constexpr AgentIntent get_intent() const noexcept {
    return intent_;
  }
  constexpr void set_intent(AgentIntent new_intent) noexcept {
    intent_ = new_intent;
  }

  [[nodiscard]] constexpr const Inventory &get_inventory() const noexcept {
    return inventory_;
  }

  [[nodiscard]] constexpr bool has_resource() const noexcept {
    return inventory_.resource != colony::world::ResourceType::None &&
           inventory_.resource_amount > 0;
  }

  constexpr void set_inventory(colony::world::ResourceType res,
                               std::uint32_t amount) noexcept {
    inventory_.resource = res;
    inventory_.resource_amount = amount;
  }

  constexpr void set_inventory(const Inventory &inventory) noexcept {
    inventory_ = inventory;
  }

  constexpr void clear_inventory() noexcept {
    inventory_.resource = colony::world::ResourceType::None;
    inventory_.resource_amount = 0;
  }

  [[nodiscard]] constexpr colony::tasks::TaskId
  claimed_task_id() const noexcept {
    return claimed_task_id_;
  }

  [[nodiscard]] constexpr bool has_claimed_task() const noexcept {
    return claimed_task_id_.is_valid();
  }

  constexpr void set_claimed_task(colony::tasks::TaskId task_id) noexcept {
    claimed_task_id_ = task_id;
  }

  constexpr void clear_claimed_task() noexcept {
    claimed_task_id_ = colony::tasks::TaskId::invalid();
  }
};

} // namespace colony::agents