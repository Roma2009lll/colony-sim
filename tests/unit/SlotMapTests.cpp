#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <colony/util/SlotMap.hpp>

namespace colony::util::testing {

TEST(SlotMapTest, DefaultStateAndEmplace) {
  SlotMap<std::string> map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  Handle h1 = map.emplace("Alpha");
  Handle h2 = map.insert("Beta");
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.is_valid(h1));
  EXPECT_TRUE(map.is_valid(h2));
  ASSERT_NE(map.find(h1), nullptr);
  ASSERT_NE(map.find(h2), nullptr);
  EXPECT_EQ(*map.find(h1), "Alpha");
  EXPECT_EQ(*map.find(h2), "Beta");
  EXPECT_EQ(map[h1], "Alpha");
  EXPECT_EQ(map[h2], "Beta");
}

TEST(SlotMapTest, EraseAndStaleHandle) {
  SlotMap<int> map;
  Handle h1 = map.emplace(100);
  Handle h2 = map.emplace(200);
  EXPECT_TRUE(map.erase(h1));
  EXPECT_EQ(map.size(), 1);
  EXPECT_FALSE(map.is_valid(h1));
  EXPECT_EQ(map.find(h1), nullptr);
  EXPECT_TRUE(map.is_valid(h2));
  ASSERT_NE(map.find(h2), nullptr);
  EXPECT_EQ(map[h2], 200);
}

TEST(SlotMapTest, SwapAndPopIntegrity) {
  SlotMap<int> map;
  Handle h0 = map.emplace(10);
  Handle h1 = map.emplace(20); 
  Handle h2 = map.emplace(30); 
  EXPECT_EQ(map.size(), 3);
  EXPECT_TRUE(map.erase(h1));
  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.is_valid(h0));
  EXPECT_TRUE(map.is_valid(h2));
  EXPECT_EQ(map[h0], 10);
  EXPECT_EQ(map[h2], 30);
  auto span = map.data();
  ASSERT_EQ(span.size(), 2);
  EXPECT_EQ(span[0], 10);
  EXPECT_EQ(span[1], 30);
}

TEST(SlotMapTest, SlotReuseAndGenerationIncrement) {
  SlotMap<std::string> map;
  Handle old_h = map.emplace("OldValue");
  const std::uint32_t original_index = old_h.index_;
  const std::uint32_t original_gen = old_h.generation_;
  EXPECT_TRUE(map.erase(old_h));
  Handle new_h = map.emplace("NewValue");
  EXPECT_EQ(new_h.index_, original_index);
  EXPECT_EQ(new_h.generation_, original_gen + 1);
  EXPECT_FALSE(map.is_valid(old_h));
  EXPECT_EQ(map.find(old_h), nullptr);
  EXPECT_TRUE(map.is_valid(new_h));
  EXPECT_EQ(map[new_h], "NewValue");
}

TEST(SlotMapTest, DataSpanMutationAndConstAccess) {
  SlotMap<int> map;
  map.emplace(1);
  map.emplace(2);
  map.emplace(3);
  for (auto& val : map.data()) {
    val *= 10;
  }
  const auto& const_map = map;
  EXPECT_EQ(const_map.size(), 3);
  auto const_span = const_map.data();
  ASSERT_EQ(const_span.size(), 3);
  EXPECT_EQ(const_span[0], 10);
  EXPECT_EQ(const_span[1], 20);
  EXPECT_EQ(const_span[2], 30);
}

TEST(SlotMapTest, InvalidAndOutOfBoundsHandles) {
  SlotMap<int> map;
  Handle default_handle{};
  Handle out_of_bounds_handle{.index_ = 999, .generation_ = 0};
  EXPECT_FALSE(map.is_valid(default_handle));
  EXPECT_FALSE(map.is_valid(out_of_bounds_handle));
  EXPECT_EQ(map.find(default_handle), nullptr);
  EXPECT_EQ(map.find(out_of_bounds_handle), nullptr);
  EXPECT_FALSE(map.erase(default_handle));
  EXPECT_FALSE(map.erase(out_of_bounds_handle));
}

} // namespace colony::util::testing