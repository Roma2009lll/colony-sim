#include <colony/util/Grid.hpp>
#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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
  EXPECT_TRUE(c2 < c1); 
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

static_assert(std::is_nothrow_move_constructible_v<Grid<int>>);
static_assert(std::is_nothrow_move_assignable_v<Grid<int>>);

TEST(GridTest, CopyIsDeepAndIndependent) {
  Grid<int> original(3, 2, 5);
  Grid<int> copy(original);
  copy(1, 1) = 99;
  EXPECT_EQ(original(1, 1), 5);
  EXPECT_EQ(copy(1, 1), 99);
  EXPECT_EQ(copy.width(), 3u);
  EXPECT_EQ(copy.height(), 2u);
}

TEST(GridTest, CopyAssignmentAndSelfAssignment) {
  Grid<int> source(2, 2, 1);
  Grid<int> target(5, 5, 2);
  target = source;
  EXPECT_EQ(target.width(), 2u);
  EXPECT_EQ(target.height(), 2u);
  EXPECT_EQ(target(1, 1), 1);
  Grid<int> &alias = target;
  target = alias;
  EXPECT_EQ(target(0, 0), 1);
}

TEST(GridTest, MoveAssignmentLeavesSourceEmpty) {
  Grid<int> source(4, 4, 7);
  Grid<int> target;
  target = std::move(source);
  EXPECT_EQ(target.size(), 16u);
  EXPECT_EQ(target(3, 3), 7);
  EXPECT_EQ(source.size(), 0u);
  EXPECT_EQ(source.width(), 0u);
  EXPECT_EQ(source.height(), 0u);
  EXPECT_EQ(source.begin(), source.end());
}

TEST(GridTest, ZeroSizedGrids) {
  Grid<int> empty;
  EXPECT_EQ(empty.size(), 0u);
  EXPECT_EQ(empty.begin(), empty.end());
  EXPECT_FALSE(empty.in_bounds(Cell{0, 0}));
  EXPECT_THROW((void)empty.at(Cell{0, 0}), std::out_of_range);
  Grid<int> no_width(0, 7);
  Grid<int> no_height(7, 0);
  EXPECT_EQ(no_width.size(), 0u);
  EXPECT_EQ(no_height.size(), 0u);
  Grid<int> copy(no_width);
  EXPECT_EQ(copy.size(), 0u);
}

TEST(GridTest, BoolGridStoresRealBools) {
  Grid<bool> grid(3, 2, false);
  static_assert(std::is_same_v<decltype(grid(0, 0)), bool &>);
  static_assert(std::is_same_v<decltype(grid.begin()), bool *>);
  int index = 0;
  for (bool &value : grid) {
    value = (index++ % 2 == 0);
  }
  EXPECT_TRUE(grid(0, 0));
  EXPECT_FALSE(grid(1, 0));
  EXPECT_EQ(grid.end() - grid.begin(), 6);
}

TEST(GridTest, NonTrivialElementType) {
  Grid<std::string> grid(2, 2, std::string("abc"));
  grid(1, 0) = "a string that is longer than the small string optimisation";
  Grid<std::string> copy(grid);
  EXPECT_EQ(copy(1, 0),
            "a string that is longer than the small string optimisation");
  EXPECT_EQ(copy(0, 0), "abc");
}

TEST(GridTest, SizeOverflowThrows) {
  EXPECT_THROW(Grid<int>(std::numeric_limits<std::size_t>::max(), 2),
               std::length_error);
}

} // namespace
} // namespace colony::util
