#include <gtest/gtest.h>

#include <colony/tasks/Task.hpp>
#include <colony/util/Grid.hpp>
#include <colony/world/Tile.hpp>

#include <type_traits>

namespace {

using colony::tasks::AgentId;
using colony::tasks::Task;
using colony::tasks::TaskError;
using colony::tasks::TaskId;
using colony::tasks::TaskPriority;
using colony::tasks::TaskResult;
using colony::tasks::TaskState;
using colony::util::Cell;
using colony::world::ResourceType;

static_assert(!std::is_same_v<TaskId, AgentId>);
static_assert(!std::is_convertible_v<TaskId, AgentId>);

static_assert(TaskResult{}.ok());
static_assert(!TaskResult{TaskError::TaskNotFound}.ok());

static_assert(Task{}.is_open());
static_assert(!Task{}.is_claimed());
static_assert(!Task{}.is_terminal());
static_assert(!Task{}.is_directive());
static_assert(Task{.state = TaskState::Completed}.is_terminal());
static_assert(Task{.state = TaskState::Cancelled}.is_terminal());
static_assert(Task{.priority = TaskPriority::Directive}.is_directive());

TEST(TaskTypesTest, StrongIdsAreDistinctAndDefaultInvalid) {
  const TaskId invalid_task;
  const AgentId invalid_agent;
  EXPECT_FALSE(invalid_task.is_valid());
  EXPECT_EQ(invalid_task, TaskId::invalid());
  EXPECT_FALSE(invalid_agent.is_valid());
  EXPECT_EQ(invalid_agent, AgentId::invalid());
  const TaskId valid_task{1};
  const AgentId valid_agent{1};
  EXPECT_TRUE(valid_task.is_valid());
  EXPECT_TRUE(valid_agent.is_valid());
}

TEST(TaskResultTest, DefaultAndErrorStates) {
  const TaskResult default_res;
  EXPECT_TRUE(default_res.ok());
  EXPECT_EQ(default_res.error, TaskError::None);
  EXPECT_FALSE(default_res.task_id.is_valid());
  const TaskResult success_with_id{TaskError::None, TaskId{7}};
  EXPECT_TRUE(success_with_id.ok());
  EXPECT_EQ(success_with_id.task_id, TaskId{7});
  const TaskError all_errors[] = {
      TaskError::TaskNotFound,    TaskError::AlreadyClaimed,
      TaskError::NotClaimed,      TaskError::WrongAgent,
      TaskError::AlreadyTerminal, TaskError::OutOfBounds,
      TaskError::NoResource,      TaskError::DuplicateDirective,
      TaskError::InvalidAgent,
  };
  for (const auto err : all_errors) {
    const TaskResult err_res{err, TaskId{3}};
    EXPECT_FALSE(err_res.ok());
    EXPECT_EQ(err_res.error, err);
    EXPECT_EQ(err_res.task_id, TaskId{3});
  }
}

TEST(TaskStructTest, DefaultInitialization) {
  const Task task;
  EXPECT_FALSE(task.id.is_valid());
  EXPECT_EQ(task.target_cell, (Cell{0, 0}));
  EXPECT_EQ(task.resource, ResourceType::None);
  EXPECT_EQ(task.priority, TaskPriority::Normal);
  EXPECT_EQ(task.state, TaskState::Open);
  EXPECT_FALSE(task.assignee.is_valid());
  EXPECT_TRUE(task.is_open());
  EXPECT_FALSE(task.is_claimed());
  EXPECT_FALSE(task.is_terminal());
  EXPECT_FALSE(task.is_directive());
}

TEST(TaskStructTest, StatePredicatesCoverAllStates) {
  Task task;
  task.state = TaskState::Open;
  EXPECT_TRUE(task.is_open());
  EXPECT_FALSE(task.is_claimed());
  EXPECT_FALSE(task.is_terminal());
  task.state = TaskState::Claimed;
  EXPECT_FALSE(task.is_open());
  EXPECT_TRUE(task.is_claimed());
  EXPECT_FALSE(task.is_terminal());
  task.state = TaskState::Completed;
  EXPECT_FALSE(task.is_open());
  EXPECT_FALSE(task.is_claimed());
  EXPECT_TRUE(task.is_terminal());
  task.state = TaskState::Cancelled;
  EXPECT_FALSE(task.is_open());
  EXPECT_FALSE(task.is_claimed());
  EXPECT_TRUE(task.is_terminal());
}

TEST(TaskStructTest, PriorityPredicates) {
  Task task;
  task.priority = TaskPriority::Normal;
  EXPECT_FALSE(task.is_directive());
  task.priority = TaskPriority::Directive;
  EXPECT_TRUE(task.is_directive());
}
} // namespace