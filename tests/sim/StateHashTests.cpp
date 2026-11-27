#include <colony/agents/Agent.hpp>
#include <colony/sim/StateHash.hpp>
#include <colony/tasks/PriorityPolicy.hpp>
#include <colony/tasks/TaskBoard.hpp>
#include <colony/world/World.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

using colony::agents::Agent;
using colony::agents::AgentIntent;
using colony::sim::compute_state_hash;
using colony::sim::StateHash;
using colony::tasks::PriorityPolicy;
using colony::tasks::TaskBoard;
using colony::util::Cell;
using colony::world::ResourceType;
using colony::world::TerrainType;
using colony::world::World;

class StateHashTest : public ::testing::Test {
protected:
  void SetUp() override {
    world_ = World(5, 5);
    world_.set_start_position(Cell{1, 1});
    world_[Cell{2, 2}].terrain = TerrainType::Ground;
    world_[Cell{2, 2}].resource = ResourceType::Food;
    world_[Cell{2, 2}].resource_amount = 10;
    world_[Cell{3, 3}].terrain = TerrainType::Storage;
    agents_.emplace_back(1, Cell{1, 1});
    agents_.emplace_back(2, Cell{2, 1});
    policy_.set_food_weight(60);
    baseline_hash_ =
        compute_state_hash(tick_, world_, agents_, task_board_, policy_);
  }

  [[nodiscard]] StateHash current_hash() const {
    return compute_state_hash(tick_, world_, agents_, task_board_, policy_);
  }

  std::uint64_t tick_{0};
  World world_{1, 1};
  std::vector<Agent> agents_{};
  TaskBoard task_board_{};
  PriorityPolicy policy_{};
  StateHash baseline_hash_{0};
};

TEST_F(StateHashTest, IsDeterministicForIdenticalState) {
  EXPECT_EQ(current_hash(), baseline_hash_);
  EXPECT_NE(baseline_hash_, 0u);
}

TEST_F(StateHashTest, SensitiveToTickChange) {
  tick_ = 1;
  EXPECT_NE(current_hash(), baseline_hash_);
}

TEST_F(StateHashTest, SensitiveToWorldTileAndStorageChanges) {
  world_[Cell{2, 2}].resource_amount = 9;
  const StateHash after_resource_change = current_hash();
  EXPECT_NE(after_resource_change, baseline_hash_);
  world_.add_stored_food(5);
  EXPECT_NE(current_hash(), after_resource_change);
}

TEST_F(StateHashTest, SensitiveToAgentPositionAndInventoryChanges) {
  agents_[0].set_position(Cell{1, 2});
  const StateHash after_move = current_hash();
  EXPECT_NE(after_move, baseline_hash_);
  agents_[0].set_intent(AgentIntent::GatherResource);
  agents_[0].set_inventory(ResourceType::Food, 3);
  EXPECT_NE(current_hash(), after_move);
}

TEST_F(StateHashTest, InvariantToAgentVectorOrdering) {
  std::reverse(agents_.begin(), agents_.end());
  EXPECT_EQ(current_hash(), baseline_hash_);
}

TEST_F(StateHashTest, IgnoresDeadAgents) {
  Agent dead_agent(99, Cell{4, 4});
  dead_agent.needs().health = 0.0f;
  dead_agent.needs().hunger = 100.0f;
  ASSERT_TRUE(dead_agent.is_dead());
  agents_.push_back(dead_agent);
  EXPECT_EQ(current_hash(), baseline_hash_);
}

TEST_F(StateHashTest, SensitiveToTaskBoardChanges) {
  const auto res = task_board_.create_directive(world_, Cell{2, 2});
  ASSERT_TRUE(res.ok());
  EXPECT_NE(current_hash(), baseline_hash_);
}

TEST_F(StateHashTest, SensitiveToPriorityPolicyChanges) {
  policy_.set_food_weight(25);
  EXPECT_NE(current_hash(), baseline_hash_);
}

} // namespace