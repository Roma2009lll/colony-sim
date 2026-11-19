#include <gtest/gtest.h>

#include <colony/tasks/PriorityPolicy.hpp>
#include <colony/world/Tile.hpp>

#include <cstdint>
#include <limits>

namespace {

using colony::tasks::PriorityPolicy;
using colony::world::ResourceType;

static_assert(PriorityPolicy{}.food_weight() == 50);
static_assert(PriorityPolicy{}.wood_weight() == 50);
static_assert(PriorityPolicy{150}.food_weight() == 100);
static_assert(PriorityPolicy{150}.wood_weight() == 0);

TEST(PriorityPolicyTest, DefaultValues) {
  const PriorityPolicy policy;
  EXPECT_EQ(policy.food_weight(), PriorityPolicy::k_default_food_weight);
  EXPECT_EQ(policy.food_weight(), 50u);
  EXPECT_EQ(policy.wood_weight(), 50u);
}

TEST(PriorityPolicyTest, InitializationWithinBounds) {
  const PriorityPolicy only_wood{0};
  EXPECT_EQ(only_wood.food_weight(), 0u);
  EXPECT_EQ(only_wood.wood_weight(), 100u);
  const PriorityPolicy only_food{100};
  EXPECT_EQ(only_food.food_weight(), 100u);
  EXPECT_EQ(only_food.wood_weight(), 0u);
  const PriorityPolicy balanced{70};
  EXPECT_EQ(balanced.food_weight(), 70u);
  EXPECT_EQ(balanced.wood_weight(), 30u);
  EXPECT_EQ(balanced.food_weight() + balanced.wood_weight(), 100u);
}

TEST(PriorityPolicyTest, ClampsValuesAboveMax) {
  const PriorityPolicy over_max{101};
  EXPECT_EQ(over_max.food_weight(), 100u);
  EXPECT_EQ(over_max.wood_weight(), 0u);
  const PriorityPolicy extreme{std::numeric_limits<std::uint32_t>::max()};
  EXPECT_EQ(extreme.food_weight(), 100u);
  EXPECT_EQ(extreme.wood_weight(), 0u);
}

TEST(PriorityPolicyTest, SetFoodWeightUpdatesAndClamps) {
  PriorityPolicy policy;
  policy.set_food_weight(25);
  EXPECT_EQ(policy.food_weight(), 25u);
  EXPECT_EQ(policy.wood_weight(), 75u);
  policy.set_food_weight(999);
  EXPECT_EQ(policy.food_weight(), 100u);
  EXPECT_EQ(policy.wood_weight(), 0u);
  policy.set_food_weight(0);
  EXPECT_EQ(policy.food_weight(), 0u);
  EXPECT_EQ(policy.wood_weight(), 100u);
}

TEST(PriorityPolicyTest, WeightForResourceType) {
  const PriorityPolicy policy{65};
  EXPECT_EQ(policy.weight_for(ResourceType::Food), 65u);
  EXPECT_EQ(policy.weight_for(ResourceType::Wood), 35u);
  EXPECT_EQ(policy.weight_for(ResourceType::None), 0u);
}

TEST(PriorityPolicyTest, EqualityComparison) {
  const PriorityPolicy a{40};
  const PriorityPolicy b{40};
  const PriorityPolicy c{60};
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

} // namespace