#include <colony/agents/Agent.hpp>
#include <gtest/gtest.h>

namespace colony::agents {
namespace {
using colony::util::Cell;
using colony::world::ResourceType;

TEST(AgentTest, InitializationDefaults) {
  constexpr Cell start_pos{10, 20};
  Agent agent{42, start_pos};
  EXPECT_EQ(agent.id(), 42);
  EXPECT_EQ(agent.position(), start_pos);
  EXPECT_EQ(agent.get_intent(), AgentIntent::Unknown);
  EXPECT_TRUE(agent.is_alive());
  EXPECT_FALSE(agent.is_dead());
  EXPECT_FALSE(agent.has_resource());
  const auto &inv = agent.get_inventory();
  EXPECT_EQ(inv.resource, ResourceType::None);
  EXPECT_EQ(inv.resource_amount, 0);
}

TEST(AgentTest, PositionAndTargetCell) {
  Agent agent{1, Cell{0, 0}};
  constexpr Cell new_pos{5, 12};
  agent.set_position(new_pos);
  EXPECT_EQ(agent.position(), new_pos);
  constexpr Cell target_pos{15, 30};
  agent.set_target_cell(target_pos);
  EXPECT_EQ(agent.get_target_cell(), target_pos);
}

TEST(AgentTest, IntentManagement) {
  Agent agent{1, Cell{0, 0}};
  agent.set_intent(AgentIntent::MoveToFood);
  EXPECT_EQ(agent.get_intent(), AgentIntent::MoveToFood);
  agent.set_intent(AgentIntent::Eat);
  EXPECT_EQ(agent.get_intent(), AgentIntent::Eat);
}

TEST(AgentTest, NeedsIntegration) {
  Agent agent{1, Cell{0, 0}};
  EXPECT_TRUE(agent.is_alive());
  EXPECT_FALSE(agent.is_dead());
  agent.needs().apply_hunger(colony::agents::Needs::kill_hunger);
  EXPECT_FALSE(agent.is_alive());
  EXPECT_TRUE(agent.is_dead());
}

TEST(AgentTest, InventoryManagement) {
  Agent agent{1, Cell{0, 0}};
  agent.set_inventory(ResourceType::Wood, 50);
  EXPECT_TRUE(agent.has_resource());
  EXPECT_EQ(agent.get_inventory().resource, ResourceType::Wood);
  EXPECT_EQ(agent.get_inventory().resource_amount, 50);
  agent.clear_inventory();
  EXPECT_FALSE(agent.has_resource());
  EXPECT_EQ(agent.get_inventory().resource, ResourceType::None);
  EXPECT_EQ(agent.get_inventory().resource_amount, 0);
  Inventory new_inv{.resource_amount = 20, .resource = ResourceType::Food};
  agent.set_inventory(new_inv);
  EXPECT_TRUE(agent.has_resource());
  EXPECT_EQ(agent.get_inventory().resource, ResourceType::Food);
  EXPECT_EQ(agent.get_inventory().resource_amount, 20);
}

} // namespace
} // namespace colony::agents