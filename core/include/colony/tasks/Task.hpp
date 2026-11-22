#pragma once

#include <cstdint>

#include <colony/util/Grid.hpp>
#include <colony/util/Id.hpp>
#include <colony/world/Tile.hpp>

namespace colony::tasks {

struct TaskTag {};
struct AgentTag {};

using TaskId = colony::util::Id<TaskTag>;
using AgentId = colony::util::Id<AgentTag>;

enum class TaskState : std::uint8_t { Open, Claimed, Completed, Cancelled };

enum class TaskPriority : std::uint8_t { Normal, Directive };

enum class TaskError : std::uint8_t {
  None,
  TaskNotFound,
  AlreadyClaimed,
  NotClaimed,
  WrongAgent,
  AlreadyTerminal,
  OutOfBounds,
  NoResource,
  DuplicateDirective,
  InvalidAgent
};

struct TaskResult {

  TaskError error{TaskError::None};

  TaskId task_id{TaskId::invalid()};

  [[nodiscard]] constexpr bool ok() const noexcept {
    return error == TaskError::None;
  }
};

struct Task {
  TaskId id{TaskId::invalid()};
  colony::util::Cell target_cell{};
  colony::world::ResourceType resource{colony::world::ResourceType::None};
  TaskPriority priority{TaskPriority::Normal};
  TaskState state{TaskState::Open};
  AgentId assignee{AgentId::invalid()};

  [[nodiscard]] constexpr bool is_open() const noexcept {
    return state == TaskState::Open;
  }

  [[nodiscard]] constexpr bool is_claimed() const noexcept {
    return state == TaskState::Claimed;
  }

  [[nodiscard]] constexpr bool is_terminal() const noexcept {
    return state == TaskState::Completed || state == TaskState::Cancelled;
  }

  [[nodiscard]] constexpr bool is_directive() const noexcept {
    return priority == TaskPriority::Directive;
  }
};

} // namespace colony::tasks