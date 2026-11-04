#include <colony/agents/Needs.hpp>
#include <gtest/gtest.h>

namespace colony::agents {
namespace {
TEST(NeedsTest, DefaultInitialization) {
  Needs needs;
  EXPECT_FLOAT_EQ(needs.hunger, 0.0f);
  EXPECT_FLOAT_EQ(needs.health, 100.0f);
  EXPECT_FALSE(needs.is_starving());
  EXPECT_FALSE(needs.is_dead());
}

TEST(NeedsTest, StarvationThreshold) {
  Needs needs;
  needs.apply_hunger(69.9f);
  EXPECT_FALSE(needs.is_starving());
  needs.apply_hunger(0.1f);
  EXPECT_TRUE(needs.is_starving());
  needs.apply_hunger(10.0f);
  EXPECT_TRUE(needs.is_starving());
}

TEST(NeedsTest, DeathByHunger) {
  Needs needs;
  needs.apply_hunger(99.9f);
  EXPECT_FALSE(needs.is_dead());
  EXPECT_FLOAT_EQ(needs.health, 100.0f);
  needs.apply_hunger(5.0f);
  EXPECT_FLOAT_EQ(needs.hunger, Needs::kill_hunger);
  EXPECT_FLOAT_EQ(needs.health, 0.0f);
  EXPECT_TRUE(needs.is_dead());
}

TEST(NeedsTest, DeathByHealth) {
  Needs needs;
  needs.health = 0.0f;
  EXPECT_TRUE(needs.is_dead());
  needs.health = -15.0f;
  EXPECT_TRUE(needs.is_dead());
}

TEST(NeedsTest, NegativeHungerClamp) {
  Needs needs;
  needs.apply_hunger(50.0f);
  needs.apply_hunger(-100.0f);
  EXPECT_FLOAT_EQ(needs.hunger, 0.0f);
  EXPECT_FALSE(needs.is_starving());
  EXPECT_FALSE(needs.is_dead());
}

} // namespace
} // namespace colony::agents