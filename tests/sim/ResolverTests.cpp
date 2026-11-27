#include <colony/agents/Agent.hpp>
#include <colony/ai/Decision.hpp>
#include <colony/metrics/Stats.hpp>
#include <colony/sim/Resolver.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/world/World.hpp>

#include <gtest/gtest.h>

#include <vector>

namespace {

using colony::agents::Agent;
using colony::ai::ActionType;
using colony::ai::Decision;
using colony::metrics::Stats;
using colony::sim::find_next_step;
using colony::sim::resolve;
using colony::sim::ResolveConfig;
using colony::tasks::TaskBoard;
using colony::tasks::TaskId;
using colony::tasks::TaskState;
using colony::util::Cell;
using colony::world::ResourceType;
using colony::world::TerrainType;
using colony::world::World;

class ResolverTest : public ::testing::Test {
protected:
  void SetUp() override {
    world_ = World(6, 6);
    world_.set_start_position(Cell{0, 0});
    world_[Cell{1, 1}].terrain = TerrainType::Storage;
    world_[Cell{4, 1}].terrain = TerrainType::Ground;
    world_[Cell{4, 1}].resource = ResourceType::Food;
    world_[Cell{4, 1}].resource_amount = 15;
    world_[Cell{1, 4}].terrain = TerrainType::Ground;
    world_[Cell{1, 4}].resource = ResourceType::Wood;
    world_[Cell{1, 4}].resource_amount = 4;
    world_[Cell{2, 1}].terrain = TerrainType::Wall;
    task_board_.sync_with_world(world_);
    agents_.emplace_back(1, Cell{1, 1});
    agents_.emplace_back(2, Cell{3, 1});
  }

  World world_{1, 1};
  TaskBoard task_board_{};
  Stats stats_{};
  std::vector<Agent> agents_{};
  ResolveConfig config_{.max_inventory = 10, .food_nutrition = 40.0f};
};

TEST_F(ResolverTest, FindNextStepNavigatesAroundWallsAndStopsAtTarget) {
  EXPECT_FALSE(find_next_step(world_, Cell{1, 1}, Cell{1, 1}).has_value());
  EXPECT_FALSE(find_next_step(world_, Cell{1, 1}, Cell{99, 99}).has_value());
  const auto step = find_next_step(world_, Cell{1, 1}, Cell{3, 1});
  ASSERT_TRUE(step.has_value());
  EXPECT_NE(*step, (Cell{2, 1}));
  EXPECT_TRUE(world_[*step].is_walkable());
}

TEST_F(ResolverTest, FindNextStepToUnwalkableTargetApproachesAdjacentCell) {
  const auto step = find_next_step(world_, Cell{0, 1}, Cell{2, 1});
  ASSERT_TRUE(step.has_value());
  EXPECT_EQ(*step, (Cell{1, 1}));
  EXPECT_FALSE(find_next_step(world_, Cell{1, 1}, Cell{2, 1}).has_value());
}

TEST_F(ResolverTest, DeadAgentReleasesClaimedTaskBeforeExecution) {
  const auto *task = task_board_.active_task_at(Cell{4, 1});
  ASSERT_NE(task, nullptr);
  const TaskId tid = task->id;
  ASSERT_TRUE(task_board_.claim(tid, agents_[0].agent_id()).ok());
  agents_[0].set_claimed_task(tid);
  agents_[0].needs().health = 0.0f;
  agents_[0].needs().hunger = 100.0f;
  ASSERT_TRUE(agents_[0].is_dead());
  std::vector<Decision> decisions(agents_.size(),
                                  Decision{.action = ActionType::Idle});
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_FALSE(agents_[0].has_claimed_task());
  const auto *updated_task = task_board_.find_task(tid);
  ASSERT_NE(updated_task, nullptr);
  EXPECT_EQ(updated_task->state, TaskState::Open);
}

TEST_F(ResolverTest, ExecutionOrderDeterministicByAgentId) {
  std::swap(agents_[0], agents_[1]);
  const auto *task = task_board_.active_task_at(Cell{4, 1});
  ASSERT_NE(task, nullptr);
  const TaskId tid = task->id;
  std::vector<Decision> decisions{
      Decision{.action = ActionType::ClaimTask, .task_id = tid},
      Decision{.action = ActionType::ClaimTask, .task_id = tid}};
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_EQ(agents_[1].id(), 1u);
  EXPECT_EQ(agents_[1].claimed_task_id(), tid);
  EXPECT_FALSE(agents_[0].has_claimed_task());
}

TEST_F(ResolverTest, GatherRespectsDistanceAndMaxInventoryAndDepletesTile) {
  std::vector<Decision> decisions{
      Decision{.action = ActionType::Idle},
      Decision{.action = ActionType::Gather, .target_cell = Cell{4, 1}}};
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_EQ(agents_[1].get_inventory().resource, ResourceType::Food);
  EXPECT_EQ(agents_[1].get_inventory().resource_amount, 10u);
  EXPECT_EQ((world_[Cell{4, 1}].resource_amount), 5u);
  agents_[0].set_position(Cell{1, 3});
  decisions[0] =
      Decision{.action = ActionType::Gather, .target_cell = Cell{1, 4}};
  decisions[1] = Decision{.action = ActionType::Idle};
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_EQ(agents_[0].get_inventory().resource, ResourceType::Wood);
  EXPECT_EQ(agents_[0].get_inventory().resource_amount, 4u);
  EXPECT_FALSE((world_[Cell{1, 4}].has_resource()));
  EXPECT_EQ((world_[Cell{1, 4}].resource_amount), 0u);
}

TEST_F(ResolverTest, DeliverDepositsResourcesAndCompletesClaimedTask) {
  const auto *task = task_board_.active_task_at(Cell{4, 1});
  ASSERT_NE(task, nullptr);
  const TaskId tid = task->id;

  ASSERT_TRUE(task_board_.claim(tid, agents_[0].agent_id()).ok());
  agents_[0].set_claimed_task(tid);
  agents_[0].set_inventory(ResourceType::Food, 7);
  agents_[0].set_position(Cell{1, 2});
  std::vector<Decision> decisions{
      Decision{.action = ActionType::Deliver, .target_cell = Cell{1, 1}},
      Decision{.action = ActionType::Idle}};
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_FALSE(agents_[0].has_resource());
  EXPECT_FALSE(agents_[0].has_claimed_task());
  EXPECT_EQ(world_.stored_food(), 7u);
  EXPECT_EQ(stats_.food_collected, 7u);
  EXPECT_EQ(stats_.tasks_completed, 1u);
  const auto *completed_task = task_board_.find_task(tid);
  ASSERT_NE(completed_task, nullptr);
  EXPECT_EQ(completed_task->state, TaskState::Completed);
}

TEST_F(ResolverTest, EatConsumesFromInventoryFirstThenFromAdjacentTile) {
  agents_[1].needs().hunger = 80.0f;
  agents_[1].set_inventory(ResourceType::Food, 2);
  agents_[1].set_position(Cell{3, 1});
  std::vector<Decision> decisions{
      Decision{.action = ActionType::Idle},
      Decision{.action = ActionType::Eat, .target_cell = Cell{4, 1}}};
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_EQ(agents_[1].get_inventory().resource_amount, 1u);
  EXPECT_EQ((world_[Cell{4, 1}].resource_amount), 15u);
  EXPECT_LT(agents_[1].needs().hunger, 80.0f);
  agents_[1].clear_inventory();
  agents_[1].needs().hunger = 80.0f;
  resolve(decisions, agents_, world_, task_board_, stats_, config_);
  EXPECT_EQ((world_[Cell{4, 1}].resource_amount), 14u);
}

} // namespace