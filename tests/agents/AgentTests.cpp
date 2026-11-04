#include <colony/agents/Agent.hpp>
#include <colony/util/SlotMap.hpp>
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

TEST(AgentTest, SetInventoryAcceptsTemporary) {
  Agent agent{1, Cell{0, 0}};
  agent.set_inventory(Inventory{.resource_amount = 5, .resource = ResourceType::Food});
  EXPECT_TRUE(agent.has_resource());
  EXPECT_EQ(agent.get_inventory().resource_amount, 5u);
}

static_assert(colony::util::SlotMapValue<Agent>);

TEST(AgentTest, StoredInSlotMap) {
  colony::util::SlotMap<Agent> agents;
  const auto first = agents.emplace(1u, Cell{0, 0});
  const auto second = agents.emplace(2u, Cell{3, 4});
  agents[first].needs().apply_hunger(Needs::kill_hunger);
  EXPECT_TRUE(agents[first].is_dead());
  EXPECT_TRUE(agents.erase(first));
  EXPECT_EQ(agents.find(first), nullptr);
  ASSERT_NE(agents.find(second), nullptr);
  EXPECT_EQ(agents[second].id(), 2u);
  EXPECT_EQ(agents[second].position(), (Cell{3, 4}));
}

} // namespace
} // namespace colony::agents
