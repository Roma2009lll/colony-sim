#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <colony/tasks/Task.hpp>
#include <colony/util/Grid.hpp>
#include <colony/world/World.hpp>

namespace colony::tasks {
class TaskBoard {
private:
  struct ActiveTaskLookup {
    Task *task{nullptr};
    TaskError error{TaskError::None};
  };

  std::vector<Task> task_board_{};
 
  [[nodiscard]] Task *find_mutable(TaskId id) {
    return const_cast<Task *>(find_task(id));
  }

  [[nodiscard]] ActiveTaskLookup find_active_mutable(TaskId id) noexcept {
    Task *task = find_mutable(id);
    if (task == nullptr) {
      return {nullptr, TaskError::TaskNotFound};
    }
    if (task->is_terminal()) {
      return {nullptr, TaskError::AlreadyTerminal};
    }
    return {task, TaskError::None};
  }

  TaskId create_task(util::Cell cell, world::ResourceType resource,
                     TaskPriority priority) {
    TaskId new_id{static_cast<std::uint32_t>(task_board_.size())};
    task_board_.emplace_back(Task{
        .id = new_id,
        .target_cell = cell,
        .resource = resource,
        .priority = priority,
    });
    return new_id;
  }

public:
  explicit TaskBoard() = default;
  TaskBoard(const TaskBoard &other) = delete;
  TaskBoard &operator=(const TaskBoard &other) = delete;

  [[nodiscard]] const Task *find_task(TaskId id) const noexcept {
    if (!id.is_valid() || id.value() >= task_board_.size()) {
      return nullptr;
    }
    return &task_board_[id.value()];
  }

  [[nodiscard]] TaskResult claim(TaskId task_id, AgentId agent_id) noexcept {
    if (!agent_id.is_valid()) {
      return TaskResult{TaskError::InvalidAgent, task_id};
    }
    auto [task, error] = find_active_mutable(task_id);
    if (error != TaskError::None) {
      return TaskResult{error, task_id};
    }
    if (task->is_claimed()) {
      return TaskResult{TaskError::AlreadyClaimed, task_id};
    }
    task->state = TaskState::Claimed;
    task->assignee = agent_id;
    return TaskResult{TaskError::None, task_id};
  }

  [[nodiscard]] TaskResult release(TaskId task_id, AgentId agent_id) noexcept {
    auto [task, error] = find_active_mutable(task_id);
    if (error != TaskError::None) {
      return TaskResult{error, task_id};
    }
    if (!task->is_claimed()) {
      return TaskResult{TaskError::NotClaimed, task_id};
    }
    if (task->assignee != agent_id) {
      return TaskResult{TaskError::WrongAgent, task_id};
    }
    task->state = TaskState::Open;
    task->assignee = AgentId::invalid();
    return TaskResult{TaskError::None, task_id};
  }

  void release_all_by_agent(AgentId agent_id) noexcept {
    for (auto &task : task_board_) {
      if (task.is_claimed() && task.assignee == agent_id) {
        task.state = TaskState::Open;
        task.assignee = AgentId::invalid();
      }
    }
  }

  [[nodiscard]] TaskResult complete(TaskId task_id, AgentId agent_id) noexcept {
    auto [task, error] = find_active_mutable(task_id);
    if (error != TaskError::None) {
      return TaskResult{error, task_id};
    }
    if (!task->is_claimed()) {
      return TaskResult{TaskError::NotClaimed, task_id};
    }
    if (task->assignee != agent_id) {
      return TaskResult{TaskError::WrongAgent, task_id};
    }
    task->state = TaskState::Completed;
    return TaskResult{TaskError::None, task_id};
  }

  [[nodiscard]] TaskResult cancel(TaskId task_id) noexcept {
    auto [task, error] = find_active_mutable(task_id);
    if (error != TaskError::None) {
      return TaskResult{error, task_id};
    }
    task->state = TaskState::Cancelled;
    task->assignee = AgentId::invalid();
    return TaskResult{TaskError::None, task_id};
  }

  [[nodiscard]] const Task *active_task_at(util::Cell cell) const noexcept {
    for (const auto &task : task_board_) {
      if (!task.is_terminal() && task.target_cell == cell) {
        return &task;
      }
    }
    return nullptr;
  }

  [[nodiscard]] bool has_active_task_at(util::Cell cell) const noexcept {
    return active_task_at(cell) != nullptr;
  }

  [[nodiscard]] constexpr std::span<const Task> task_board() const noexcept {
    return task_board_;
  }

  [[nodiscard]] std::size_t size() const noexcept { return task_board_.size(); }

  [[nodiscard]] bool empty() const noexcept { return task_board_.empty(); }

  void sync_with_world(const world::World &world) {
    for (auto &task : task_board_) {
      if (task.is_terminal()) {
        continue;
      }
      if (!world.in_bounds(task.target_cell) ||
          !(world[task.target_cell].has_resource()) ||
          world[task.target_cell].resource != task.resource) {
        static_cast<void>(cancel(task.id));
      }
    }
    for (size_t x = 0; x < world.width(); x++) {
      for (size_t y = 0; y < world.height(); y++) {
        util::Cell cell{static_cast<decltype(util::Cell::x)>(x),
                        static_cast<decltype(util::Cell::y)>(y)};
        if (world[cell].has_resource() && (!has_active_task_at(cell))) {
          create_task(cell, world[cell].resource, TaskPriority::Normal);
        }
      }
    }
  }

  [[nodiscard]] TaskResult create_directive(const world::World &world,
                                            util::Cell cell) {
    if (!world.in_bounds(cell)) {
      return TaskResult{TaskError::OutOfBounds};
    }
    if (!world[cell].has_resource()) {
      return TaskResult{TaskError::NoResource};
    }
    const Task *exising = active_task_at(cell);
    if (exising != nullptr && exising->priority == TaskPriority::Directive) {
      return TaskResult{TaskError::DuplicateDirective};
    }
    if (exising != nullptr) {
      static_cast<void>(cancel(exising->id));
    }
    TaskId id =
        create_task(cell, world[cell].resource, TaskPriority::Directive);
    return TaskResult{TaskError::None, id};
  }
};
} // namespace colony::tasks