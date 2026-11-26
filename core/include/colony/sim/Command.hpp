#pragma once

#include <cstdint>
#include <variant>

#include <colony/tasks/Task.hpp>
#include <colony/util/Grid.hpp>

namespace colony::sim {

struct MarkDirective {
  util::Cell cell_{};
  constexpr bool operator==(const MarkDirective &) const noexcept = default;
};

struct CancelTask {
  tasks::TaskId task_id_{tasks::TaskId::invalid()};
  constexpr bool operator==(const CancelTask &) const noexcept = default;
};

struct SetBalance {
  std::uint32_t food_weight_{50}; 
  constexpr bool operator==(const SetBalance &) const noexcept = default;
};

using Command = std::variant<MarkDirective, CancelTask, SetBalance>;

enum class CommandRejectReason : std::uint8_t {
  None,
  OutOfBounds,
  NoResource,
  DuplicateDirective,
  TaskNotFound,
  AlreadyTerminal,
  InvalidBalance
};

struct CommandReceipt {
  std::uint64_t tick_{0};
  Command command_{};
  bool accepted_{false};
  CommandRejectReason reason_{CommandRejectReason::None};
  [[nodiscard]] constexpr bool ok() const noexcept {
    return accepted_ && reason_ == CommandRejectReason::None;
  }
  constexpr bool operator==(const CommandReceipt &) const noexcept = default;
};

} // namespace colony::sim