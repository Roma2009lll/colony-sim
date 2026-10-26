#include <colony/util/Rng.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace colony::util {
namespace {
TEST(RngTest, DeterminismWithSameSeed) {
  Rng rng1(12345ULL);
  Rng rng2(12345ULL);

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(rng1.next_u64(), rng2.next_u64());
  }
}

TEST(RngTest, AgentDeterminismAndUniqueness) {
  std::uint64_t seed = 42;
  std::uint64_t tick = 100;
  std::uint64_t agent1_id = 1;
  std::uint64_t agent2_id = 2;
  Rng rng_a1 = Rng::for_agent(seed, tick, agent1_id);
  Rng rng_a1_dup = Rng::for_agent(seed, tick, agent1_id);
  Rng rng_a2 = Rng::for_agent(seed, tick, agent2_id);
  EXPECT_EQ(rng_a1.next_u64(), rng_a1_dup.next_u64());
  EXPECT_NE(rng_a1.next_u64(), rng_a2.next_u64());
}

TEST(RngTest, RangeBounds) {
  Rng rng(999ULL);
  for (int i = 0; i < 1000; ++i) {
    std::int32_t val = rng.range(-10, 10);
    EXPECT_GE(val, -10);
    EXPECT_LE(val, 10);
  }
  EXPECT_EQ(rng.range(5, 5), 5);
}

TEST(RngTest, UniformBounds) {
  Rng rng(777ULL);
  for (int i = 0; i < 1000; ++i) {
    double val = rng.uniform();
    EXPECT_GE(val, 0.0);
    EXPECT_LT(val, 1.0);
  }
}

TEST(RngTest, ChanceEdgeCases) {
  Rng rng(123ULL);
  EXPECT_FALSE(rng.chance(0.0));
  EXPECT_FALSE(rng.chance(-0.5));
  EXPECT_TRUE(rng.chance(1.0));
  EXPECT_TRUE(rng.chance(1.5));
  double nan_val = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(rng.chance(nan_val));
}

TEST(RngTest, ChanceStatisticalDistribution) {
  Rng rng(42ULL);
  int true_count = 0;
  constexpr int iterations = 10000;
  for (int i = 0; i < iterations; ++i) {
    if (rng.chance(0.5)) {
      ++true_count;
    }
  }
  EXPECT_NEAR(true_count, 5000, 300);
}

} // namespace
} // namespace colony::util