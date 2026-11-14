#include "colony/pathfinding/BFS.hpp"
#include "colony/pathfinding/GraphConcept.hpp"
#include "colony/pathfinding/GridGraph.hpp"
#include "colony/util/Grid.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

namespace colony::pathfinding::test {
namespace {

struct WalkableZero {
  constexpr bool operator()(int cell_value) const noexcept {
    return cell_value == 0;
  }
};

using TestGridGraph = GridGraph<int, WalkableZero>;

void ExpectValidGridPath(const TestGridGraph &graph, const PathView &view,
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

TEST(BfsPathFinderTest, OutOfBoundsStartOrGoalReturnsEmpty) {
  util::Grid<int> grid(4, 4);
  TestGridGraph graph(grid, WalkableZero{});
  BfsPathFinder bfs;
  const NodeId valid = graph.to_node({1, 1});
  const NodeId out_of_range = static_cast<NodeId>(graph.node_count());
  EXPECT_FALSE(bfs.find_path(graph, out_of_range, valid).found());
  EXPECT_FALSE(bfs.find_path(graph, valid, out_of_range).found());
  EXPECT_FALSE(bfs.find_path(graph, k_invalid_node, valid).found());
  EXPECT_FALSE(bfs.find_path(graph, valid, k_invalid_node).found());
}

TEST(BfsPathFinderTest, UnwalkableStartOrGoalReturnsEmpty) {
  util::Grid<int> grid(4, 4);
  const util::Cell wall{2, 2};
  grid[wall] = 1;
  TestGridGraph graph(grid, WalkableZero{});
  BfsPathFinder bfs;
  const NodeId free_node = graph.to_node({0, 0});
  const NodeId wall_node = graph.to_node(wall);
  EXPECT_FALSE(bfs.find_path(graph, wall_node, free_node).found());
  EXPECT_FALSE(bfs.find_path(graph, free_node, wall_node).found());
  EXPECT_FALSE(bfs.find_path(graph, wall_node, wall_node).found());
}

TEST(BfsPathFinderTest, StartEqualsGoalOnWalkableCell) {
  util::Grid<int> grid(4, 4);
  TestGridGraph graph(grid, WalkableZero{});
  BfsPathFinder bfs;
  const NodeId node = graph.to_node({2, 1});
  const PathView view = bfs.find_path(graph, node, node);
  ASSERT_TRUE(view.found());
  ASSERT_EQ(view.path_.size(), 1U);
  EXPECT_EQ(view.path_[0], node);
  EXPECT_EQ(view.total_cost_, Cost{0});
  EXPECT_EQ(view.expanded_nodes_, 0U);
}

TEST(BfsPathFinderTest, FindsShortestPathOnOpenGrid) {
  util::Grid<int> grid(5, 5);
  TestGridGraph graph(grid, WalkableZero{});
  BfsPathFinder bfs;
  const NodeId start = graph.to_node({0, 0});
  const NodeId goal = graph.to_node({3, 2});
  const PathView view = bfs.find_path(graph, start, goal);
  ExpectValidGridPath(graph, view, start, goal);
  EXPECT_EQ(view.total_cost_, Cost{5});
  EXPECT_EQ(view.path_.size(), 6U);
  EXPECT_GT(view.expanded_nodes_, 0U);
}

TEST(BfsPathFinderTest, NavigatesAroundObstacles) {
  util::Grid<int> grid(5, 5);
  for (int y = 0; y < 4; ++y) {
    grid[util::Cell{2, y}] = 1;
  }
  TestGridGraph graph(grid, WalkableZero{});
  BfsPathFinder bfs;
  const NodeId start = graph.to_node({0, 0});
  const NodeId goal = graph.to_node({4, 0});
  const PathView view = bfs.find_path(graph, start, goal);
  ExpectValidGridPath(graph, view, start, goal);
  EXPECT_EQ(view.total_cost_, Cost{12});
  EXPECT_EQ(view.path_.size(), 13U);
}

TEST(BfsPathFinderTest, UnreachableGoalReturnsEmpty) {
  util::Grid<int> grid(5, 5);
  for (int y = 0; y < 5; ++y) {
    grid[util::Cell{2, y}] = 1;
  }
  TestGridGraph graph(grid, WalkableZero{});
  BfsPathFinder bfs;
  const NodeId start = graph.to_node({0, 2});
  const NodeId goal = graph.to_node({4, 2});
  const PathView view = bfs.find_path(graph, start, goal);
  EXPECT_FALSE(view.found());
  EXPECT_TRUE(view.path_.empty());
  EXPECT_EQ(view.total_cost_, Cost{0});
}

TEST(BfsPathFinderTest, ReusesBuffersAcrossMultipleSearchesAndGridResize) {
  BfsPathFinder bfs;
  {
    util::Grid<int> small_grid(3, 3);
    TestGridGraph small_graph(small_grid, WalkableZero{});
    const NodeId s = small_graph.to_node({0, 0});
    const NodeId g = small_graph.to_node({2, 2});
    const PathView v1 = bfs.find_path(small_graph, s, g);
    ExpectValidGridPath(small_graph, v1, s, g);
    EXPECT_EQ(v1.total_cost_, Cost{4});
  }
  {
    util::Grid<int> small_grid(3, 3);
    TestGridGraph small_graph(small_grid, WalkableZero{});
    const NodeId s = small_graph.to_node({2, 2});
    const NodeId g = small_graph.to_node({0, 0});
    const PathView v2 = bfs.find_path(small_graph, s, g);
    ExpectValidGridPath(small_graph, v2, s, g);
    EXPECT_EQ(v2.total_cost_, Cost{4});
  }
  {
    util::Grid<int> large_grid(10, 10);
    TestGridGraph large_graph(large_grid, WalkableZero{});
    const NodeId s = large_graph.to_node({0, 0});
    const NodeId g = large_graph.to_node({9, 9});
    const PathView v3 = bfs.find_path(large_graph, s, g);
    ExpectValidGridPath(large_graph, v3, s, g);
    EXPECT_EQ(v3.total_cost_, Cost{18});
  }
}

} // namespace
} // namespace colony::pathfinding::test