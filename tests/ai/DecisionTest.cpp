#include <gtest/gtest.h>

#include <colony/agents/Agent.hpp>
#include <colony/ai/Decision.hpp>
#include <colony/tasks/Task.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/util/Grid.hpp>
#include <colony/util/Rng.hpp>
#include <colony/world/Tile.hpp>
#include <colony/world/World.hpp>

#include <cstdint>
#include <type_traits>
#include <vector>

namespace {

using colony::agents::Agent;
using colony::ai::ActionType;
using colony::ai::decide;
using colony::ai::Decision;
using colony::ai::PriorityPolicy;
using colony::tasks::TaskBoard;
using colony::tasks::TaskError;
using colony::tasks::TaskId;
using colony::util::Cell;
using colony::util::Rng;
using colony::world::ResourceType;
using colony::world::TerrainType;
using colony::world::World;

static_assert(
    std::is_same_v<colony::ai::PriorityPolicy, colony::tasks::PriorityPolicy>,
    "decide() must use the same PriorityPolicy as the rest of the "
    "task system");

class DecisionTest : public ::testing::Test {
protected:
  World world{10, 10, Cell{0, 0}};
  TaskBoard board;
  Rng rng{42};

  void SetUp() override { world[Cell{0, 0}].terrain = TerrainType::Storage; }

  void place_resource(Cell cell, ResourceType type, std::uint32_t amount = 5) {
    world[cell].resource = type;
    world[cell].resource_amount = amount;
  }

  static void make_hungry(Agent &agent) {
    while (!agent.needs().is_hungry() && !agent.is_dead()) {
      agent.needs().tick();
    }
  }

  static void make_dead(Agent &agent) {
    while (!agent.is_dead()) {
      agent.needs().tick();
    }
  }
};

TEST_F(DecisionTest, DeadAgentAlwaysReturnsIdle) {
  Agent agent{1, Cell{2, 2}};
  place_resource(Cell{2, 3}, ResourceType::Food, 10);
  board.sync_with_world(world);
  ASSERT_EQ(board.create_directive(world, Cell{2, 3}).error, TaskError::None);
  make_dead(agent);
  ASSERT_TRUE(agent.is_dead());
  const Decision d = decide(agent, world, board, PriorityPolicy{100}, rng);
  EXPECT_EQ(d.action, ActionType::Idle);
}

TEST_F(DecisionTest, HungryAgentMovesToNearestFoodAndEatsInReach) {
  Agent agent{1, Cell{5, 5}};
  make_hungry(agent);
  ASSERT_TRUE(agent.needs().is_hungry());
  place_resource(Cell{5, 6}, ResourceType::Food, 0);
  place_resource(Cell{1, 1}, ResourceType::Food, 5);
  place_resource(Cell{5, 8}, ResourceType::Food, 5);
  place_resource(Cell{5, 4}, ResourceType::Wood, 5);
  board.sync_with_world(world);
  ASSERT_EQ(board.create_directive(world, Cell{5, 4}).error, TaskError::None);
  const Decision step = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(step.action, ActionType::MoveStep);
  EXPECT_EQ(step.target_cell, (Cell{5, 8}));
  agent.set_position(Cell{5, 7});
  const Decision eat = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(eat.action, ActionType::Eat);
  EXPECT_EQ(eat.target_cell, (Cell{5, 8}));
}

TEST_F(DecisionTest, HungryAgentEatsFromStorageOnlyWhenFoodStored) {
  Agent agent{1, Cell{0, 1}};
  make_hungry(agent);
  const Decision idle = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(idle.action, ActionType::Idle);
  world.add_stored_food(3);
  const Decision eat = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(eat.action, ActionType::Eat);
  EXPECT_EQ(eat.target_cell, (Cell{0, 0}));
}

TEST_F(DecisionTest, HungryAgentFallsBackToWorkWhenNoFoodExists) {
  Agent agent{1, Cell{3, 3}};
  make_hungry(agent);
  place_resource(Cell{4, 3}, ResourceType::Food, 5);
  board.sync_with_world(world);
  world[Cell{4, 3}].resource = ResourceType::Wood;
  board.sync_with_world(world);
  const Decision d = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d.action, ActionType::ClaimTask);
  EXPECT_EQ(d.target_cell, (Cell{4, 3}));
}

TEST_F(DecisionTest, AgentWithResourceMovesToNearestStorageAndDelivers) {
  world[Cell{6, 6}].terrain = TerrainType::Storage;
  Agent agent{1, Cell{6, 3}};
  agent.set_inventory(ResourceType::Wood, 4);
  const Decision step = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(step.action, ActionType::MoveStep);
  EXPECT_EQ(step.target_cell, (Cell{6, 6}));
  agent.set_position(Cell{6, 5});
  const Decision deliver = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(deliver.action, ActionType::Deliver);
  EXPECT_EQ(deliver.target_cell, (Cell{6, 6}));
}

TEST_F(DecisionTest, AgentWithClaimedTaskMovesAndGathersInReach) {
  const Cell wood_cell{4, 4};
  place_resource(wood_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  const TaskId task_id = board.active_task_at(wood_cell)->id;
  Agent agent{1, Cell{1, 4}};
  ASSERT_EQ(board.claim(task_id, agent.agent_id()).error, TaskError::None);
  const Decision step = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(step.action, ActionType::MoveStep);
  EXPECT_EQ(step.target_cell, wood_cell);
  EXPECT_EQ(step.task_id, task_id);
  agent.set_position(Cell{3, 4});
  const Decision gather = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(gather.action, ActionType::Gather);
  EXPECT_EQ(gather.target_cell, wood_cell);
  EXPECT_EQ(gather.task_id, task_id);
}

TEST_F(DecisionTest, DirectiveTakesPriorityOverCloserNormalTask) {
  const Cell near_normal{2, 3};
  const Cell far_directive{8, 8};
  place_resource(near_normal, ResourceType::Food, 5);
  place_resource(far_directive, ResourceType::Wood, 5);
  board.sync_with_world(world);
  const auto dir_res = board.create_directive(world, far_directive);
  ASSERT_EQ(dir_res.error, TaskError::None);
  Agent agent{1, Cell{2, 2}};
  const Decision d = decide(agent, world, board, PriorityPolicy{100}, rng);
  EXPECT_EQ(d.action, ActionType::ClaimTask);
  EXPECT_EQ(d.target_cell, far_directive);
  EXPECT_EQ(d.task_id, dir_res.task_id);
}

TEST_F(DecisionTest, PriorityPolicySelectsResourceType) {
  const Cell food_cell{2, 4};
  const Cell wood_cell{4, 2};
  place_resource(food_cell, ResourceType::Food, 5);
  place_resource(wood_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  Agent agent{1, Cell{2, 2}};
  const Decision food_d = decide(agent, world, board, PriorityPolicy{100}, rng);
  EXPECT_EQ(food_d.action, ActionType::ClaimTask);
  EXPECT_EQ(food_d.target_cell, food_cell);
  const Decision wood_d = decide(agent, world, board, PriorityPolicy{0}, rng);
  EXPECT_EQ(wood_d.action, ActionType::ClaimTask);
  EXPECT_EQ(wood_d.target_cell, wood_cell);
}

TEST_F(DecisionTest, PriorityPolicyWeightControlsShareOfChosenResource) {
  const Cell food_cell{2, 4};
  const Cell wood_cell{4, 2};
  place_resource(food_cell, ResourceType::Food, 5);
  place_resource(wood_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  Agent agent{1, Cell{2, 2}};
  const auto food_share = [&](PriorityPolicy policy) {
    Rng local_rng{2024};
    constexpr int k_draws = 4000;
    int food = 0;
    for (int i = 0; i < k_draws; ++i) {
      if (decide(agent, world, board, policy, local_rng).target_cell ==
          food_cell) {
        ++food;
      }
    }
    return static_cast<double>(food) / k_draws;
  };
  EXPECT_DOUBLE_EQ(food_share(PriorityPolicy{0}), 0.0);
  EXPECT_DOUBLE_EQ(food_share(PriorityPolicy{100}), 1.0);
  EXPECT_DOUBLE_EQ(food_share(PriorityPolicy{1000}), 1.0);
  EXPECT_NEAR(food_share(PriorityPolicy{70}), 0.7, 0.05);
  EXPECT_NEAR(food_share(PriorityPolicy{25}), 0.25, 0.05);
}

TEST_F(DecisionTest, FallsBackToSecondaryResourceWhenPreferredUnavailable) {
  const Cell wood_cell{4, 2};
  place_resource(wood_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  Agent agent{1, Cell{2, 2}};
  const Decision d = decide(agent, world, board, PriorityPolicy{100}, rng);
  EXPECT_EQ(d.action, ActionType::ClaimTask);
  EXPECT_EQ(d.target_cell, wood_cell);
}

TEST_F(DecisionTest, TieBreaksEqualDistanceByLowestTaskId) {
  const Cell first_cell{2, 4};
  const Cell second_cell{4, 2};
  place_resource(first_cell, ResourceType::Wood, 5);
  place_resource(second_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  const TaskId first_id = board.active_task_at(first_cell)->id;
  const TaskId second_id = board.active_task_at(second_cell)->id;
  ASSERT_LT(first_id.value(), second_id.value());
  Agent agent{1, Cell{2, 2}};
  const Decision d = decide(agent, world, board, PriorityPolicy{0}, rng);
  EXPECT_EQ(d.action, ActionType::ClaimTask);
  EXPECT_EQ(d.task_id, first_id);
  EXPECT_EQ(d.target_cell, first_cell);
}

TEST_F(DecisionTest, OnlyOneColonistPerDirective) {
  const Cell directive_cell{6, 6};
  const Cell normal_cell{1, 2};
  place_resource(directive_cell, ResourceType::Wood, 5);
  place_resource(normal_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  const auto dir = board.create_directive(world, directive_cell);
  ASSERT_EQ(dir.error, TaskError::None);
  Agent first{1, Cell{2, 2}};
  Agent second{2, Cell{2, 2}};
  ASSERT_EQ(board.claim(dir.task_id, first.agent_id()).error, TaskError::None);
  const Decision d1 = decide(first, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d1.target_cell, directive_cell);
  EXPECT_EQ(d1.task_id, dir.task_id);
  const Decision d2 = decide(second, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d2.action, ActionType::ClaimTask);
  EXPECT_EQ(d2.target_cell, normal_cell);
}

TEST_F(DecisionTest, NewDirectiveDoesNotPreemptAlreadyClaimedTask) {
  const Cell claimed_cell{1, 2};
  const Cell directive_cell{8, 8};
  place_resource(claimed_cell, ResourceType::Wood, 5);
  place_resource(directive_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  Agent agent{1, Cell{2, 2}};
  const TaskId claimed_id = board.active_task_at(claimed_cell)->id;
  ASSERT_EQ(board.claim(claimed_id, agent.agent_id()).error, TaskError::None);
  ASSERT_EQ(board.create_directive(world, directive_cell).error,
            TaskError::None);
  const Decision d = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d.task_id, claimed_id);
  EXPECT_EQ(d.target_cell, claimed_cell);
}

TEST_F(DecisionTest, DirectiveOnClaimedCellDropsClaimAndAgentTakesDirective) {
  const Cell cell{4, 4};
  place_resource(cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  Agent agent{1, Cell{2, 2}};
  ASSERT_EQ(board.claim(board.active_task_at(cell)->id, agent.agent_id()).error,
            TaskError::None);
  const auto dir = board.create_directive(world, cell);
  ASSERT_EQ(dir.error, TaskError::None);
  const Decision d = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d.action, ActionType::ClaimTask);
  EXPECT_EQ(d.task_id, dir.task_id);
}

TEST_F(DecisionTest,
       AgentCarryingResourceDeliversEvenIfItsDirectiveWasExhausted) {
  const Cell cell{5, 5};
  place_resource(cell, ResourceType::Wood, 1);
  const auto dir = board.create_directive(world, cell);
  ASSERT_EQ(dir.error, TaskError::None);
  Agent agent{1, Cell{5, 4}};
  ASSERT_EQ(board.claim(dir.task_id, agent.agent_id()).error, TaskError::None);
  agent.set_inventory(ResourceType::Wood, 1);
  world[cell].resource_amount = 0;
  board.sync_with_world(world);
  ASSERT_EQ(board.find_task(dir.task_id)->state,
            colony::tasks::TaskState::Cancelled);
  const Decision d = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d.action, ActionType::MoveStep);
  EXPECT_EQ(d.target_cell, (Cell{0, 0}));
  EXPECT_FALSE(d.task_id.is_valid());
}

TEST_F(DecisionTest, FoodShortageDoesNotChangePrioritiesByItself) {
  const Cell food_cell{2, 4};
  const Cell wood_cell{4, 2};
  place_resource(food_cell, ResourceType::Food, 5);
  place_resource(wood_cell, ResourceType::Wood, 5);
  board.sync_with_world(world);
  Agent agent{1, Cell{2, 2}};
  while (agent.needs().hunger < 69.0f) {
    agent.needs().tick();
  }
  ASSERT_FALSE(agent.needs().is_hungry());
  const auto targets = [&](PriorityPolicy policy) {
    Rng local_rng{99};
    std::vector<Cell> result;
    for (int i = 0; i < 200; ++i) {
      result.push_back(
          decide(agent, world, board, policy, local_rng).target_cell);
    }
    return result;
  };
  const auto with_empty_storage = targets(PriorityPolicy{0});
  world.add_stored_food(1000);
  const auto with_full_storage = targets(PriorityPolicy{0});
  EXPECT_EQ(with_empty_storage, with_full_storage);
  for (const Cell &c : with_empty_storage) {
    EXPECT_EQ(c, wood_cell);
  }
  const auto food_biased = targets(PriorityPolicy{100});
  for (const Cell &c : food_biased) {
    EXPECT_EQ(c, food_cell);
  }
}

TEST_F(DecisionTest, DecideIsDeterministicAndLeavesInputsUntouched) {
  for (int i = 1; i < 9; ++i) {
    place_resource(Cell{i, 3},
                   (i % 2) ? ResourceType::Food : ResourceType::Wood, 4);
  }
  board.sync_with_world(world);
  const auto dir = board.create_directive(world, Cell{8, 3});
  ASSERT_EQ(dir.error, TaskError::None);
  ASSERT_EQ(board.claim(dir.task_id, colony::tasks::AgentId{99}).error,
            TaskError::None);
  Agent agent{1, Cell{4, 5}};
  agent.needs().apply_hunger(10.0f);
  const auto board_before = std::vector<colony::tasks::Task>(
      board.task_board().begin(), board.task_board().end());
  const auto pos_before = agent.position();
  const auto hunger_before = agent.needs().hunger;
  const auto food_before = world.stored_food();
  Rng a{7};
  Rng b{7};
  for (int i = 0; i < 500; ++i) {
    const Decision x = decide(agent, world, board, PriorityPolicy{50}, a);
    const Decision y = decide(agent, world, board, PriorityPolicy{50}, b);
    ASSERT_EQ(x.action, y.action);
    ASSERT_EQ(x.target_cell, y.target_cell);
    ASSERT_EQ(x.task_id, y.task_id);
  }
  ASSERT_EQ(board.size(), board_before.size());
  for (std::size_t i = 0; i < board_before.size(); ++i) {
    const auto &now = board.task_board()[i];
    EXPECT_EQ(now.id, board_before[i].id);
    EXPECT_EQ(now.state, board_before[i].state);
    EXPECT_EQ(now.assignee, board_before[i].assignee);
    EXPECT_EQ(now.priority, board_before[i].priority);
    EXPECT_EQ(now.target_cell, board_before[i].target_cell);
  }
  EXPECT_EQ(agent.position(), pos_before);
  EXPECT_EQ(agent.needs().hunger, hunger_before);
  EXPECT_EQ(world.stored_food(), food_before);
}

TEST_F(DecisionTest, ReturnsIdleWhenNoTasksAvailable) {
  Agent agent{1, Cell{2, 2}};
  const Decision d = decide(agent, world, board, PriorityPolicy{50}, rng);
  EXPECT_EQ(d.action, ActionType::Idle);
  EXPECT_FALSE(d.task_id.is_valid());
}
} // namespace