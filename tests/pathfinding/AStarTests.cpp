#include "colony/pathfinding/AStar.hpp"
#include "colony/pathfinding/BFS.hpp"
#include "colony/pathfinding/GraphConcept.hpp"
#include "colony/pathfinding/GridGraph.hpp"
#include "colony/util/Grid.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace colony::pathfinding::test {
namespace {

struct WalkableZero {
  constexpr bool operator()(int cell_value) const noexcept {
    return cell_value == 0;
  }
};

using TestGridGraph = GridGraph<int, WalkableZero>;

class WeightedGridGraph {
private:
  const util::Grid<Cost> &costs_;

public:
  explicit WeightedGridGraph(const util::Grid<Cost> &costs) : costs_(costs) {}

  constexpr NodeId to_node(util::Cell cell) const noexcept {
    return static_cast<NodeId>(static_cast<std::size_t>(cell.y) *
                                   costs_.width() +
                               static_cast<std::size_t>(cell.x));
  }

  constexpr util::Cell to_cell(NodeId id) const noexcept {
    return util::Cell{static_cast<std::int32_t>(id % costs_.width()),
                      static_cast<std::int32_t>(id / costs_.width())};
  }

  constexpr std::size_t node_count() const noexcept { return costs_.size(); }

  constexpr bool is_walkable(NodeId u) const noexcept {
    if (static_cast<std::size_t>(u) >= node_count()) {
      return false;
    }
    return costs_[to_cell(u)] > 0;
  }

  constexpr Cost heuristic(NodeId u, NodeId v) const noexcept {
    const util::Cell c0 = to_cell(u);
    const util::Cell c1 = to_cell(v);
    return static_cast<Cost>(std::abs(c0.x - c1.x) + std::abs(c0.y - c1.y));
  }

  template <typename Callback>
  void for_each_neighbor(NodeId u, Callback &&func) const {
    if (static_cast<std::size_t>(u) >= node_count()) {
      return;
    }
    static constexpr std::array<util::Cell, 4> k_dirs{
        {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    const util::Cell current = to_cell(u);
    for (const auto &dir : k_dirs) {
      const util::Cell next = current + dir;
      if (costs_.in_bounds(next) && costs_[next] > 0) {
        func(to_node(next), costs_[next]);
      }
    }
  }
};

static_assert(GraphLike<WeightedGridGraph>);

void ExpectValidUnitGridPath(const TestGridGraph &graph, const PathView &view,
                             NodeId expected_start, NodeId expected_goal) {
  ASSERT_TRUE(view.found());
  ASSERT_FALSE(view.path_.empty());
  EXPECT_EQ(view.path_.front(), expected_start);
  EXPECT_EQ(view.path_.back(), expected_goal);
  EXPECT_EQ(view.total_cost_, static_cast<Cost>(view.path_.size() - 1));
  for (std::size_t i = 0; i < view.path_.size(); ++i) {
    EXPECT_TRUE(graph.is_walkable(view.path_[i]))
        << "Step " << i << " is blocked";
    if (i > 0) {
      EXPECT_EQ(graph.heuristic(view.path_[i - 1], view.path_[i]), Cost{1})
          << "Non-adjacent step between index " << (i - 1) << " and " << i;
    }
  }
}

TEST(AStarPathFinderTest, OutOfBoundsStartOrGoalReturnsEmpty) {
  util::Grid<int> grid(4, 4);
  TestGridGraph graph(grid, WalkableZero{});
  AStarPathFinder astar;
  const NodeId valid = graph.to_node({1, 1});
  const NodeId out_of_range = static_cast<NodeId>(graph.node_count());
  EXPECT_FALSE(astar.find_path(graph, out_of_range, valid).found());
  EXPECT_FALSE(astar.find_path(graph, valid, out_of_range).found());
  EXPECT_FALSE(astar.find_path(graph, k_invalid_node, valid).found());
  EXPECT_FALSE(astar.find_path(graph, valid, k_invalid_node).found());
}

TEST(AStarPathFinderTest, UnwalkableStartOrGoalReturnsEmpty) {
  util::Grid<int> grid(4, 4);
  const util::Cell wall{2, 2};
  grid[wall] = 1;
  TestGridGraph graph(grid, WalkableZero{});
  AStarPathFinder astar;
  const NodeId free_node = graph.to_node({0, 0});
  const NodeId wall_node = graph.to_node(wall);
  EXPECT_FALSE(astar.find_path(graph, wall_node, free_node).found());
  EXPECT_FALSE(astar.find_path(graph, free_node, wall_node).found());
  EXPECT_FALSE(astar.find_path(graph, wall_node, wall_node).found());
}

TEST(AStarPathFinderTest, StartEqualsGoalOnWalkableCell) {
  util::Grid<int> grid(4, 4);
  TestGridGraph graph(grid, WalkableZero{});
  AStarPathFinder astar;
  const NodeId node = graph.to_node({2, 1});
  const PathView view = astar.find_path(graph, node, node);
  ASSERT_TRUE(view.found());
  ASSERT_EQ(view.path_.size(), 1U);
  EXPECT_EQ(view.path_[0], node);
  EXPECT_EQ(view.total_cost_, Cost{0});
  EXPECT_EQ(view.expanded_nodes_, 0U);
}

TEST(AStarPathFinderTest, FindsOptimalPathAndExpandsFewerNodesThanBfs) {
  util::Grid<int> grid(10, 10);
  TestGridGraph graph(grid, WalkableZero{});
  AStarPathFinder astar;
  BfsPathFinder bfs;
  const NodeId start = graph.to_node({1, 1});
  const NodeId goal = graph.to_node({8, 8});
  const PathView astar_view = astar.find_path(graph, start, goal);
  const PathView bfs_view = bfs.find_path(graph, start, goal);
  ExpectValidUnitGridPath(graph, astar_view, start, goal);
  EXPECT_EQ(astar_view.total_cost_, bfs_view.total_cost_);
  EXPECT_EQ(astar_view.total_cost_, Cost{14});
  EXPECT_LT(astar_view.expanded_nodes_, bfs_view.expanded_nodes_);
}

TEST(AStarPathFinderTest, NavigatesAroundObstaclesAndHandlesUnreachableGoal) {
  util::Grid<int> grid(5, 5);
  for (int y = 0; y < 4; ++y) {
    grid[util::Cell{2, y}] = 1;
  }
  TestGridGraph graph(grid, WalkableZero{});
  AStarPathFinder astar;
  const NodeId start = graph.to_node({0, 0});
  const NodeId goal = graph.to_node({4, 0});
  const PathView open_view = astar.find_path(graph, start, goal);
  ExpectValidUnitGridPath(graph, open_view, start, goal);
  EXPECT_EQ(open_view.total_cost_, Cost{12});
  grid[util::Cell{2, 4}] = 1;
  const PathView blocked_view = astar.find_path(graph, start, goal);
  EXPECT_FALSE(blocked_view.found());
  EXPECT_TRUE(blocked_view.path_.empty());
}

TEST(AStarPathFinderTest, AvoidsExpensiveTerrainAndRelaxesSuboptimalPaths) {
  util::Grid<Cost> costs(3, 3, Cost{1});
  costs[util::Cell{1, 0}] = Cost{10};
  WeightedGridGraph graph(costs);
  AStarPathFinder astar;
  const NodeId start = graph.to_node({0, 0});
  const NodeId goal = graph.to_node({2, 0});
  const PathView view = astar.find_path(graph, start, goal);
  ASSERT_TRUE(view.found());
  EXPECT_EQ(view.total_cost_, Cost{4});
  ASSERT_EQ(view.path_.size(), 5U);
  EXPECT_EQ(view.path_[0], graph.to_node({0, 0}));
  EXPECT_EQ(view.path_[1], graph.to_node({0, 1}));
  EXPECT_EQ(view.path_[2], graph.to_node({1, 1}));
  EXPECT_EQ(view.path_[3], graph.to_node({2, 1}));
  EXPECT_EQ(view.path_[4], graph.to_node({2, 0}));
}

TEST(AStarPathFinderTest, ReusesBuffersAcrossMultipleSearchesAndGridResize) {
  AStarPathFinder astar;
  {
    util::Grid<int> small_grid(3, 3);
    TestGridGraph small_graph(small_grid, WalkableZero{});
    const NodeId s = small_graph.to_node({0, 0});
    const NodeId g = small_graph.to_node({2, 2});
    const PathView v1 = astar.find_path(small_graph, s, g);
    ExpectValidUnitGridPath(small_graph, v1, s, g);
    EXPECT_EQ(v1.total_cost_, Cost{4});
  }
  {
    util::Grid<int> small_grid(3, 3);
    TestGridGraph small_graph(small_grid, WalkableZero{});
    const NodeId s = small_graph.to_node({2, 2});
    const NodeId g = small_graph.to_node({0, 0});
    const PathView v2 = astar.find_path(small_graph, s, g);
    ExpectValidUnitGridPath(small_graph, v2, s, g);
    EXPECT_EQ(v2.total_cost_, Cost{4});
  }
  {
    util::Grid<int> large_grid(12, 12);
    TestGridGraph large_graph(large_grid, WalkableZero{});
    const NodeId s = large_graph.to_node({0, 0});
    const NodeId g = large_graph.to_node({11, 11});
    const PathView v3 = astar.find_path(large_graph, s, g);
    ExpectValidUnitGridPath(large_graph, v3, s, g);
    EXPECT_EQ(v3.total_cost_, Cost{22});
  }
}

} // namespace
} // namespace colony::pathfinding::test