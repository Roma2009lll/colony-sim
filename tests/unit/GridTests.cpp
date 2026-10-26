#include <colony/util/Grid.hpp>
#include <gtest/gtest.h>

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace colony::util {
namespace {
TEST(CellTest, ArithmeticAndComparison) {
  constexpr Cell c1{3, 5};
  constexpr Cell c2{1, 2};
  constexpr Cell sum = c1 + c2;
  EXPECT_EQ(sum.x, 4);
  EXPECT_EQ(sum.y, 7);
  constexpr Cell diff = c1 - c2;
  EXPECT_EQ(diff.x, 2);
  EXPECT_EQ(diff.y, 3);
  EXPECT_EQ(c1, (Cell{3, 5}));
  EXPECT_NE(c1, c2);
  EXPECT_TRUE(c2 < c1); // x: 1 < 3
}

TEST(CellTest, UnorderedContainerHashSupport) {
  std::unordered_map<Cell, std::string> cell_map;
  Cell pos{10, -20};
  cell_map[pos] = "Home";
  EXPECT_EQ(cell_map[(Cell{10, -20})], "Home");
  std::unordered_set<Cell> visited;
  visited.insert(Cell{0, 0});
  visited.insert(Cell{0, 0});
  EXPECT_EQ(visited.size(), 1u);
}

TEST(GridTest, ConstructionAndDimensions) {
  Grid<int> grid(10, 5, 42);
  EXPECT_EQ(grid.width(), 10u);
  EXPECT_EQ(grid.height(), 5u);
  EXPECT_EQ(grid.size(), 50u);
  EXPECT_EQ(grid(0, 0), 42);
  EXPECT_EQ(grid(9, 4), 42);
}

TEST(GridTest, ElementAccessAndModification) {
  Grid<int> grid(5, 5, 0);
  grid(2, 3) = 100;
  EXPECT_EQ(grid[(Cell{2, 3})], 100);
  grid[Cell{1, 1}] = 50;
  EXPECT_EQ(grid(1, 1), 50);
  grid.at(Cell{4, 4}) = 99;
  EXPECT_EQ(grid.at(Cell{4, 4}), 99);
}

TEST(GridTest, BoundsCheckingAndExceptions) {
  Grid<int> grid(5, 5);
  EXPECT_TRUE(grid.in_bounds(Cell{0, 0}));
  EXPECT_TRUE(grid.in_bounds(Cell{4, 4}));
  EXPECT_FALSE(grid.in_bounds(Cell{-1, 0}));
  EXPECT_FALSE(grid.in_bounds(Cell{0, -1}));
  EXPECT_FALSE(grid.in_bounds(Cell{5, 0}));
  EXPECT_FALSE(grid.in_bounds(Cell{0, 5}));
  EXPECT_THROW((void)grid.at(Cell{-1, 2}), std::out_of_range);
  EXPECT_THROW((void)grid.at(Cell{5, 5}), std::out_of_range);
}

TEST(GridTest, MoveSemantics) {
  Grid<int> original(4, 4, 7);
  original(1, 1) = 99;
  Grid<int> moved(std::move(original));
  EXPECT_EQ(moved.width(), 4u);
  EXPECT_EQ(moved.height(), 4u);
  EXPECT_EQ(moved(1, 1), 99);
  EXPECT_EQ(original.width(), 0u);
  EXPECT_EQ(original.height(), 0u);
  EXPECT_EQ(original.size(), 0u);
}

TEST(GridTest, SpecializationForBool) {
  Grid<bool> bool_grid(3, 3, false);
  EXPECT_FALSE(bool_grid(1, 1));
  bool_grid(1, 1) = true;
  EXPECT_TRUE(bool_grid(1, 1));
  bool_grid[Cell{0, 0}] = true;
  EXPECT_TRUE(bool_grid.at(Cell{0, 0}));
}

TEST(GridTest, IterationSupport) {
  Grid<int> grid(3, 3, 0);
  int counter = 1;
  for (int &val : grid) {
    val = counter++;
  }
  EXPECT_EQ(grid(0, 0), 1);
  EXPECT_EQ(grid(2, 2), 9);
}

} // namespace
} // namespace colony::util
