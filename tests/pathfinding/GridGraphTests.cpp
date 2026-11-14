#include "colony/pathfinding/GraphConcept.hpp"
#include "colony/pathfinding/GridGraph.hpp"
#include "colony/util/Grid.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace colony::pathfinding::test {
namespace {

struct WalkableZero {
  constexpr bool operator()(int cell_value) const noexcept {
    return cell_value == 0;
  }
};

using TestGridGraph = GridGraph<int, WalkableZero>;

static_assert(GraphLike<TestGridGraph>);

TEST(GridGraphTest, NodeCountAndCoordinateConversions) {
  util::Grid<int> grid(4, 3);
  TestGridGraph graph(grid, WalkableZero{});
  EXPECT_EQ(graph.node_count(), 12U);
  const std::vector<std::pair<util::Cell, NodeId>> cases = {
      {{0, 0}, 0}, {{3, 0}, 3}, {{0, 1}, 4}, {{2, 1}, 6}, {{3, 2}, 11},
  };
  for (const auto &[cell, expected_id] : cases) {
    EXPECT_EQ(graph.to_node(cell), expected_id);
    EXPECT_EQ(graph.to_cell(expected_id), cell);
  }
}

TEST(GridGraphTest, IsWalkableHandlesWallsAndOutOfBounds) {
  util::Grid<int> grid(3, 3);
  grid[util::Cell{1, 1}] = 1;
  TestGridGraph graph(grid, WalkableZero{});
  EXPECT_TRUE(graph.is_walkable(graph.to_node({0, 0})));
  EXPECT_FALSE(graph.is_walkable(graph.to_node({1, 1})));
  EXPECT_FALSE(graph.is_walkable(static_cast<NodeId>(graph.node_count())));
  EXPECT_FALSE(graph.is_walkable(k_invalid_node));
}

TEST(GridGraphTest, HeuristicComputesManhattanDistance) {
  util::Grid<int> grid(5, 5);
  TestGridGraph graph(grid, WalkableZero{});
  const NodeId a = graph.to_node({1, 1});
  const NodeId b = graph.to_node({4, 3});
  EXPECT_EQ(graph.heuristic(a, a), Cost{0});
  EXPECT_EQ(graph.heuristic(a, b), Cost{5});
  EXPECT_EQ(graph.heuristic(b, a), Cost{5});
}

TEST(GridGraphTest, ForEachNeighborFromCenterReturnsFourCardinalNeighbors) {
  util::Grid<int> grid(3, 3);
  TestGridGraph graph(grid, WalkableZero{});
  std::vector<NodeId> neighbors;
  graph.for_each_neighbor(graph.to_node({1, 1}), [&](NodeId next, Cost cost) {
    EXPECT_EQ(cost, Cost{1});
    neighbors.push_back(next);
  });
  std::sort(neighbors.begin(), neighbors.end());
  const std::vector<NodeId> expected = {
      graph.to_node({1, 0}),
      graph.to_node({0, 1}),
      graph.to_node({2, 1}),
      graph.to_node({1, 2}),
  };

  EXPECT_EQ(neighbors, expected);
}

TEST(GridGraphTest, ForEachNeighborRespectsBoundsAndObstacles) {
  util::Grid<int> grid(3, 3);
  grid[util::Cell{1, 0}] = 1;
  TestGridGraph graph(grid, WalkableZero{});
  std::vector<NodeId> neighbors;
  graph.for_each_neighbor(graph.to_node({0, 0}), [&](NodeId next, Cost cost) {
    EXPECT_EQ(cost, Cost{1});
    neighbors.push_back(next);
  });
  ASSERT_EQ(neighbors.size(), 1U);
  EXPECT_EQ(neighbors[0], graph.to_node({0, 1}));
  std::size_t invalid_calls = 0;
  graph.for_each_neighbor(k_invalid_node,
                          [&](NodeId, Cost) { ++invalid_calls; });
  EXPECT_EQ(invalid_calls, 0U);
}

TEST(GridGraphTest, EmptyGridHasNoNodesAndConversionsAreSafe) {
  util::Grid<int> grid;
  TestGridGraph graph(grid, WalkableZero{});
  EXPECT_EQ(graph.node_count(), 0U);
  EXPECT_FALSE(graph.is_walkable(0));
  EXPECT_EQ(graph.to_cell(0), (util::Cell{0, 0}));
  EXPECT_EQ(graph.heuristic(0, 0), Cost{0});
  std::size_t calls = 0;
  graph.for_each_neighbor(0, [&](NodeId, Cost) { ++calls; });
  EXPECT_EQ(calls, 0U);
}

} // namespace
} // namespace colony::pathfinding::test