#include <gtest/gtest.h>

#include <colony/tasks/Task.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/util/Grid.hpp>
#include <colony/world/Tile.hpp>
#include <colony/world/World.hpp>

#include <cstdint>

namespace {

using colony::tasks::AgentId;
using colony::tasks::Task;
using colony::tasks::TaskBoard;
using colony::tasks::TaskError;
using colony::tasks::TaskId;
using colony::tasks::TaskPriority;
using colony::tasks::TaskState;
using colony::util::Cell;
using colony::world::ResourceType;
using colony::world::TerrainType;
using colony::world::World;

class TaskBoardTest : public ::testing::Test {
protected:
  World world{5, 5, Cell{0, 0}};
  TaskBoard board;

  void SetUp() override { world[Cell{0, 0}].terrain = TerrainType::Storage; }

  void place_resource(Cell cell, ResourceType type, std::uint32_t amount = 5) {
    world[cell].resource = type;
    world[cell].resource_amount = amount;
  }
};

TEST_F(TaskBoardTest, InitialStateAndInvalidLookups) {
  EXPECT_TRUE(board.empty());
  EXPECT_EQ(board.size(), 0u);
  EXPECT_TRUE(board.task_board().empty());
  EXPECT_EQ(board.find_task(TaskId::invalid()), nullptr);
  EXPECT_EQ(board.find_task(TaskId{0}), nullptr);
  EXPECT_EQ(board.active_task_at(Cell{1, 1}), nullptr);
  EXPECT_FALSE(board.has_active_task_at(Cell{1, 1}));
}

TEST_F(TaskBoardTest, SyncWithWorldCreatesAndDeduplicatesNormalTasks) {
  const Cell food_cell{1, 2};
  const Cell wood_cell{3, 4};
  place_resource(food_cell, ResourceType::Food, 10);
  place_resource(wood_cell, ResourceType::Wood, 8);
  place_resource(Cell{2, 2}, ResourceType::Food, 0);
  board.sync_with_world(world);
  ASSERT_EQ(board.size(), 2u);
  EXPECT_FALSE(board.empty());
  const Task *food_task = board.active_task_at(food_cell);
  ASSERT_NE(food_task, nullptr);
  EXPECT_EQ(food_task->resource, ResourceType::Food);
  EXPECT_EQ(food_task->priority, TaskPriority::Normal);
  EXPECT_EQ(food_task->state, TaskState::Open);
  EXPECT_EQ(food_task->assignee, AgentId::invalid());
  const Task *wood_task = board.active_task_at(wood_cell);
  ASSERT_NE(wood_task, nullptr);
  EXPECT_EQ(wood_task->resource, ResourceType::Wood);
  EXPECT_EQ(wood_task->priority, TaskPriority::Normal);
  board.sync_with_world(world);
  EXPECT_EQ(board.size(), 2u);
}

TEST_F(TaskBoardTest, SyncWithWorldCancelsDepletedOrChangedResources) {
  const Cell food_cell{1, 1};
  const Cell wood_cell{2, 2};
  place_resource(food_cell, ResourceType::Food, 5);
  place_resource(wood_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  ASSERT_EQ(board.size(), 2u);
  const TaskId food_id = board.active_task_at(food_cell)->id;
  const TaskId wood_id = board.active_task_at(wood_cell)->id;
  ASSERT_EQ(board.claim(food_id, AgentId{1}).error, TaskError::None);
  world[food_cell].resource_amount = 0;
  world[wood_cell].resource = ResourceType::Food;
  board.sync_with_world(world);
  const Task *cancelled_food = board.find_task(food_id);
  ASSERT_NE(cancelled_food, nullptr);
  EXPECT_EQ(cancelled_food->state, TaskState::Cancelled);
  EXPECT_EQ(cancelled_food->assignee, AgentId::invalid());
  EXPECT_FALSE(board.has_active_task_at(food_cell));
  const Task *cancelled_wood = board.find_task(wood_id);
  ASSERT_NE(cancelled_wood, nullptr);
  EXPECT_EQ(cancelled_wood->state, TaskState::Cancelled);
  const Task *new_task = board.active_task_at(wood_cell);
  ASSERT_NE(new_task, nullptr);
  EXPECT_NE(new_task->id, wood_id);
  EXPECT_EQ(new_task->resource, ResourceType::Food);
  EXPECT_EQ(new_task->state, TaskState::Open);
}

TEST_F(TaskBoardTest, SyncWithWorldCreatesNewTaskIfPreviousWasCompleted) {
  const Cell cell{2, 3};
  place_resource(cell, ResourceType::Wood, 10);
  board.sync_with_world(world);
  const TaskId first_id = board.active_task_at(cell)->id;
  ASSERT_EQ(board.claim(first_id, AgentId{1}).error, TaskError::None);
  ASSERT_EQ(board.complete(first_id, AgentId{1}).error, TaskError::None);
  EXPECT_FALSE(board.has_active_task_at(cell));
  board.sync_with_world(world);
  ASSERT_EQ(board.size(), 2u);
  const Task *second_task = board.active_task_at(cell);
  ASSERT_NE(second_task, nullptr);
  EXPECT_NE(second_task->id, first_id);
  EXPECT_EQ(second_task->state, TaskState::Open);
}

TEST_F(TaskBoardTest, CreateDirectiveValidationAndNormalTaskOverride) {
  const Cell empty_cell{1, 1};
  const Cell resource_cell{2, 2};
  place_resource(resource_cell, ResourceType::Wood, 5);
  EXPECT_EQ(board.create_directive(world, Cell{99, 99}).error,
            TaskError::OutOfBounds);
  EXPECT_EQ(board.create_directive(world, empty_cell).error,
            TaskError::NoResource);
  board.sync_with_world(world);
  const TaskId normal_id = board.active_task_at(resource_cell)->id;
  ASSERT_EQ(board.claim(normal_id, AgentId{7}).error, TaskError::None);
  const auto res = board.create_directive(world, resource_cell);
  ASSERT_EQ(res.error, TaskError::None);
  EXPECT_NE(res.task_id, normal_id);
  const Task *old_task = board.find_task(normal_id);
  ASSERT_NE(old_task, nullptr);
  EXPECT_EQ(old_task->state, TaskState::Cancelled);
  EXPECT_EQ(old_task->assignee, AgentId::invalid());
  const Task *directive = board.active_task_at(resource_cell);
  ASSERT_NE(directive, nullptr);
  EXPECT_EQ(directive->id, res.task_id);
  EXPECT_EQ(directive->priority, TaskPriority::Directive);
  EXPECT_EQ(directive->state, TaskState::Open);
  EXPECT_EQ(board.create_directive(world, resource_cell).error,
            TaskError::DuplicateDirective);
}

TEST_F(TaskBoardTest, ClaimLifecycleAndErrors) {
  place_resource(Cell{1, 1}, ResourceType::Food, 5);
  board.sync_with_world(world);
  const TaskId id = board.active_task_at(Cell{1, 1})->id;
  EXPECT_EQ(board.claim(TaskId::invalid(), AgentId{1}).error,
            TaskError::TaskNotFound);
  EXPECT_EQ(board.claim(TaskId{42}, AgentId{1}).error, TaskError::TaskNotFound);
  EXPECT_EQ(board.claim(id, AgentId::invalid()).error, TaskError::InvalidAgent);
  EXPECT_TRUE(board.find_task(id)->is_open());
  const auto ok = board.claim(id, AgentId{1});
  EXPECT_EQ(ok.error, TaskError::None);
  EXPECT_EQ(ok.task_id, id);
  const Task *task = board.find_task(id);
  ASSERT_NE(task, nullptr);
  EXPECT_EQ(task->state, TaskState::Claimed);
  EXPECT_EQ(task->assignee, AgentId{1});
  EXPECT_EQ(board.claim(id, AgentId{1}).error, TaskError::AlreadyClaimed);
  EXPECT_EQ(board.claim(id, AgentId{2}).error, TaskError::AlreadyClaimed);
  ASSERT_EQ(board.cancel(id).error, TaskError::None);
  EXPECT_EQ(board.claim(id, AgentId{1}).error, TaskError::AlreadyTerminal);
}

TEST_F(TaskBoardTest, ReleaseAndReleaseAllByAgent) {
  place_resource(Cell{1, 1}, ResourceType::Food, 5);
  place_resource(Cell{1, 2}, ResourceType::Food, 5);
  place_resource(Cell{1, 3}, ResourceType::Wood, 5);
  board.sync_with_world(world);
  const TaskId t1 = board.active_task_at(Cell{1, 1})->id;
  const TaskId t2 = board.active_task_at(Cell{1, 2})->id;
  const TaskId t3 = board.active_task_at(Cell{1, 3})->id;
  EXPECT_EQ(board.release(t1, AgentId{1}).error, TaskError::NotClaimed);
  ASSERT_EQ(board.claim(t1, AgentId{1}).error, TaskError::None);
  ASSERT_EQ(board.claim(t2, AgentId{1}).error, TaskError::None);
  ASSERT_EQ(board.claim(t3, AgentId{2}).error, TaskError::None);
  EXPECT_EQ(board.release(t1, AgentId{2}).error, TaskError::WrongAgent);
  EXPECT_EQ(board.release(t1, AgentId{1}).error, TaskError::None);
  EXPECT_EQ(board.find_task(t1)->state, TaskState::Open);
  EXPECT_EQ(board.find_task(t1)->assignee, AgentId::invalid());
  ASSERT_EQ(board.claim(t1, AgentId{1}).error, TaskError::None);
  board.release_all_by_agent(AgentId{1});
  EXPECT_EQ(board.find_task(t1)->state, TaskState::Open);
  EXPECT_EQ(board.find_task(t1)->assignee, AgentId::invalid());
  EXPECT_EQ(board.find_task(t2)->state, TaskState::Open);
  EXPECT_EQ(board.find_task(t2)->assignee, AgentId::invalid());
  EXPECT_EQ(board.find_task(t3)->state, TaskState::Claimed);
  EXPECT_EQ(board.find_task(t3)->assignee, AgentId{2});
}

TEST_F(TaskBoardTest, CompleteAndCancelLifecycle) {
  place_resource(Cell{1, 1}, ResourceType::Food, 5);
  place_resource(Cell{2, 2}, ResourceType::Wood, 5);
  board.sync_with_world(world);
  const TaskId t1 = board.active_task_at(Cell{1, 1})->id;
  const TaskId t2 = board.active_task_at(Cell{2, 2})->id;
  EXPECT_EQ(board.complete(t1, AgentId{1}).error, TaskError::NotClaimed);
  ASSERT_EQ(board.claim(t1, AgentId{1}).error, TaskError::None);
  EXPECT_EQ(board.complete(t1, AgentId{2}).error, TaskError::WrongAgent);
  EXPECT_EQ(board.complete(t1, AgentId{1}).error, TaskError::None);
  EXPECT_EQ(board.find_task(t1)->state, TaskState::Completed);
  EXPECT_EQ(board.find_task(t1)->assignee, AgentId{1});
  EXPECT_FALSE(board.has_active_task_at(Cell{1, 1}));
  EXPECT_EQ(board.complete(t1, AgentId{1}).error, TaskError::AlreadyTerminal);
  EXPECT_EQ(board.release(t1, AgentId{1}).error, TaskError::AlreadyTerminal);
  EXPECT_EQ(board.cancel(t1).error, TaskError::AlreadyTerminal);
  EXPECT_EQ(board.cancel(t2).error, TaskError::None);
  EXPECT_EQ(board.find_task(t2)->state, TaskState::Cancelled);
  EXPECT_EQ(board.find_task(t2)->assignee, AgentId::invalid());
}

TEST_F(TaskBoardTest, DeathOfAssigneeReturnsDirectiveToWaiting) {
  const Cell cell{2, 2};
  place_resource(cell, ResourceType::Wood, 5);
  const auto res = board.create_directive(world, cell);
  ASSERT_EQ(res.error, TaskError::None);
  ASSERT_EQ(board.claim(res.task_id, AgentId{3}).error, TaskError::None);
  board.release_all_by_agent(AgentId{3});
  const Task *directive = board.find_task(res.task_id);
  ASSERT_NE(directive, nullptr);
  EXPECT_EQ(directive->state, TaskState::Open);
  EXPECT_EQ(directive->priority, TaskPriority::Directive);
  EXPECT_EQ(directive->assignee, AgentId::invalid());
  EXPECT_EQ(board.active_task_at(cell), directive);
  EXPECT_EQ(board.size(), 1u);
  EXPECT_EQ(board.claim(res.task_id, AgentId{4}).error, TaskError::None);
}

TEST_F(TaskBoardTest, DepletedNodeCancelsDirectiveAndRegrowthMakesNormalTask) {
  const Cell cell{2, 2};
  place_resource(cell, ResourceType::Food, 5);
  const auto res = board.create_directive(world, cell);
  ASSERT_EQ(res.error, TaskError::None);
  ASSERT_EQ(board.claim(res.task_id, AgentId{1}).error, TaskError::None);
  world[cell].resource_amount = 0;
  board.sync_with_world(world);
  const Task *directive = board.find_task(res.task_id);
  ASSERT_NE(directive, nullptr);
  EXPECT_EQ(directive->state, TaskState::Cancelled);
  EXPECT_EQ(directive->assignee, AgentId::invalid());
  EXPECT_FALSE(board.has_active_task_at(cell));
  EXPECT_EQ(board.create_directive(world, cell).error, TaskError::NoResource);
  world[cell].resource_amount = 5;
  board.sync_with_world(world);
  const Task *regrown = board.active_task_at(cell);
  ASSERT_NE(regrown, nullptr);
  EXPECT_EQ(regrown->priority, TaskPriority::Normal);
  EXPECT_NE(regrown->id, res.task_id);
}

TEST_F(TaskBoardTest, DirectiveCanBeCreatedAgainAfterItWasCompleted) {
  const Cell cell{3, 3};
  place_resource(cell, ResourceType::Wood, 9);
  const auto first = board.create_directive(world, cell);
  ASSERT_EQ(first.error, TaskError::None);
  ASSERT_EQ(board.claim(first.task_id, AgentId{1}).error, TaskError::None);
  ASSERT_EQ(board.complete(first.task_id, AgentId{1}).error, TaskError::None);
  const auto second = board.create_directive(world, cell);
  ASSERT_EQ(second.error, TaskError::None);
  EXPECT_NE(second.task_id, first.task_id);
}

} // namespace
