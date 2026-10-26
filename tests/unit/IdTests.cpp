#include <colony/util/Id.hpp>
#include <gtest/gtest.h>

#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace colony::util {
namespace {
struct AgentTag {};
struct BuildingTag {};
using AgentId = Id<AgentTag>;
using BuildingId = Id<BuildingTag>;
using CustomId = Id<AgentTag, std::uint64_t>;

} // namespace

TEST(IdTest, DefaultAndInvalidState) {
  AgentId default_id;
  EXPECT_FALSE(default_id.is_valid());
  EXPECT_EQ(default_id, AgentId::invalid());
  EXPECT_EQ(default_id.value(), std::numeric_limits<std::uint32_t>::max());
}

TEST(IdTest, ValidCreationAndGetter) {
  constexpr AgentId id{42};
  EXPECT_TRUE(id.is_valid());
  EXPECT_EQ(id.value(), 42u);
}

TEST(IdTest, ComparisonOperators) {
  AgentId id1{10};
  AgentId id2{10};
  AgentId id3{20};
  EXPECT_EQ(id1, id2);
  EXPECT_NE(id1, id3);
  EXPECT_LT(id1, id3);
  EXPECT_LE(id1, id2);
  EXPECT_GT(id3, id1);
}

TEST(IdTest, StrictTypeSafetyAtCompileTime) {
  static_assert(!std::is_same_v<AgentId, BuildingId>);
  static_assert(!std::is_convertible_v<AgentId, BuildingId>);
}

TEST(IdTest, UnorderedContainerSupport) {
  std::unordered_map<AgentId, std::string> agent_names;
  AgentId id1{1};
  AgentId id2{2};
  agent_names[id1] = "Worker_1";
  agent_names[id2] = "Worker_2";
  EXPECT_EQ(agent_names[id1], "Worker_1");
  EXPECT_EQ(agent_names[id2], "Worker_2");
  std::unordered_set<AgentId> unique_ids;
  unique_ids.insert(id1);
  unique_ids.insert(id1);
  EXPECT_EQ(unique_ids.size(), 1u);
}

TEST(IdTest, CustomUnderlyingType) {
  CustomId id{100ULL};
  EXPECT_TRUE(id.is_valid());
  EXPECT_EQ(id.value(), 100ULL);
}

} // namespace colony::util