#include <colony/agents/Agent.hpp>
#include <colony/sim/CommandLog.hpp>
#include <colony/sim/Simulation.hpp>
#include <colony/world/World.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

using colony::agents::Agent;
using colony::sim::CancelTask;
using colony::sim::CommandLog;
using colony::sim::CommandRejectReason;
using colony::sim::MarkDirective;
using colony::sim::SetBalance;
using colony::sim::Simulation;
using colony::sim::SimulationConfig;
using colony::tasks::TaskId;
using colony::util::Cell;
using colony::world::ResourceType;
using colony::world::TerrainType;
using colony::world::World;

class SimulationTest : public ::testing::Test {
protected:
  void SetUp() override {
    world_ = World(6, 6, Cell{1, 1});
    world_[Cell{1, 1}].terrain = TerrainType::Storage;
    world_[Cell{3, 1}].terrain = TerrainType::Ground;
    world_[Cell{3, 1}].resource = ResourceType::Food;
    world_[Cell{3, 1}].resource_amount = 30;
    world_[Cell{1, 4}].terrain = TerrainType::Ground;
    world_[Cell{1, 4}].resource = ResourceType::Wood;
    world_[Cell{1, 4}].resource_amount = 30;
    agents_.emplace_back(1, Cell{1, 1});
    agents_.emplace_back(2, Cell{2, 1});
    config_.resource_regrow_chance = 0.0;
  }

  World world_{1, 1};
  std::vector<Agent> agents_{};
  SimulationConfig config_{};
};

TEST_F(SimulationTest, DeterministicAcrossIdenticalRuns) {
  CommandLog log1;
  log1.add(0, MarkDirective{Cell{3, 1}});
  log1.add(5, SetBalance{75});
  CommandLog log2;
  log2.add(0, MarkDirective{Cell{3, 1}});
  log2.add(5, SetBalance{75});
  Simulation sim1(world_, agents_, 42, std::move(log1), config_);
  Simulation sim2(world_, agents_, 42, std::move(log2), config_);
  sim1.run(40);
  sim2.run(40);
  EXPECT_EQ(sim1.tick(), 40u);
  EXPECT_EQ(sim1.state_hash(), sim2.state_hash());
  EXPECT_EQ(sim1.stats(), sim2.stats());
  EXPECT_EQ(sim1.stats().final_state_hash, sim1.state_hash());
}

TEST_F(SimulationTest, ConstructorSortsAgentsByIdForDeterminism) {
  std::vector<Agent> reversed_agents = agents_;
  std::reverse(reversed_agents.begin(), reversed_agents.end());
  Simulation sim_normal(world_, agents_, 123, {}, config_);
  Simulation sim_reversed(world_, std::move(reversed_agents), 123, {}, config_);
  EXPECT_EQ(sim_normal.state_hash(), sim_reversed.state_hash());
  sim_normal.run(20);
  sim_reversed.run(20);
  EXPECT_EQ(sim_normal.state_hash(), sim_reversed.state_hash());
}

TEST_F(SimulationTest, AppliesCommandsAndRecordsReceipts) {
  CommandLog log;
  log.add(0, MarkDirective{Cell{3, 1}});
  log.add(0, MarkDirective{Cell{3, 1}});
  log.add(0, MarkDirective{Cell{99, 99}});
  log.add(1, SetBalance{80});
  log.add(1, SetBalance{999});
  log.add(1, CancelTask{TaskId{9999}});
  Simulation sim(world_, agents_, 7, std::move(log), config_);
  sim.run(2);
  ASSERT_EQ(sim.receipts().size(), 6u);
  EXPECT_TRUE(sim.receipts()[0].ok());
  EXPECT_FALSE(sim.receipts()[1].ok());
  EXPECT_EQ(sim.receipts()[1].reason_, CommandRejectReason::DuplicateDirective);
  EXPECT_FALSE(sim.receipts()[2].ok());
  EXPECT_EQ(sim.receipts()[2].reason_, CommandRejectReason::OutOfBounds);
  EXPECT_TRUE(sim.receipts()[3].ok());
  EXPECT_EQ(sim.priority_policy().food_weight(), 80u);
  EXPECT_FALSE(sim.receipts()[4].ok());
  EXPECT_EQ(sim.receipts()[4].reason_, CommandRejectReason::InvalidBalance);
  EXPECT_FALSE(sim.receipts()[5].ok());
  EXPECT_EQ(sim.receipts()[5].reason_, CommandRejectReason::TaskNotFound);
  EXPECT_EQ(sim.stats().directives_accepted, 1u);
  EXPECT_EQ(sim.stats().directives_rejected, 2u);
}

TEST_F(SimulationTest, SpawnsNewColonistWhenStorageMeetsBirthCost) {
  config_.birth_food_cost = 10;
  config_.birth_wood_cost = 5;
  config_.max_agents = 5;
  world_.add_stored_food(12);
  world_.add_stored_wood(7);
  Simulation sim(world_, agents_, 1, {}, config_);
  ASSERT_EQ(sim.agents().size(), 2u);
  sim.step();
  EXPECT_EQ(sim.agents().size(), 3u);
  EXPECT_EQ(sim.stats().colonists_born, 1u);
  EXPECT_EQ(sim.world().stored_food(), 2u);
  EXPECT_EQ(sim.world().stored_wood(), 2u);
  EXPECT_EQ(sim.agents().back().id(), 3u);
  EXPECT_EQ(sim.agents().back().position(), sim.world().start_position());
}

TEST_F(SimulationTest, StopsEarlyWhenAllColonistsStarve) {
  world_[Cell{3, 1}].reset();
  config_.hunger_rate = 100.0f;
  Simulation sim(world_, agents_, 1, {}, config_);
  sim.run(100);
  EXPECT_TRUE(sim.agents().empty());
  EXPECT_EQ(sim.stats().colonists_died, 2u);
  EXPECT_TRUE(sim.is_finished(100));
  EXPECT_LT(sim.tick(), 100u);
}

} // namespace