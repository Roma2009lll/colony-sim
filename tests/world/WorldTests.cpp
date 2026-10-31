#include <colony/world/World.hpp>
#include <gtest/gtest.h>

namespace colony::world {

TEST(WorldTest, ConstructionAndDimensions) {
  World default_world;
  EXPECT_EQ(default_world.width(), 0);
  EXPECT_EQ(default_world.height(), 0);
  EXPECT_EQ(default_world.stored_food(), 0);
  EXPECT_EQ(default_world.stored_wood(), 0);
  World world(10, 5, util::Cell{2, 3});
  EXPECT_EQ(world.width(), 10);
  EXPECT_EQ(world.height(), 5);
  EXPECT_EQ(world.start_position(), (util::Cell{2, 3}));
}

TEST(WorldTest, BoundsAndTileAccess) {
  World world(5, 5);
  EXPECT_TRUE(world.in_bounds(util::Cell{0, 0}));
  EXPECT_TRUE(world.in_bounds(util::Cell{4, 4}));
  EXPECT_FALSE(world.in_bounds(util::Cell{-1, 0}));
  EXPECT_FALSE(world.in_bounds(util::Cell{0, -1}));
  EXPECT_FALSE(world.in_bounds(util::Cell{5, 2}));
  EXPECT_FALSE(world.in_bounds(util::Cell{2, 5}));
  util::Cell cell{1, 2};
  world[cell].terrain = TerrainType::Wall;
  EXPECT_EQ(world[cell].terrain, TerrainType::Wall);
  EXPECT_FALSE(world[cell].is_walkable());
  world.at(cell).terrain = TerrainType::Storage;
  EXPECT_EQ(world.at(cell).terrain, TerrainType::Storage);
}

TEST(WorldTest, ResourceStorage) {
  World world(5, 5);
  EXPECT_EQ(world.stored_food(), 0);
  EXPECT_EQ(world.stored_wood(), 0);
  world.add_stored_food(50);
  world.add_stored_wood(30);
  EXPECT_EQ(world.stored_food(), 50);
  EXPECT_EQ(world.stored_wood(), 30);
  world.add_stored_food(20);
  EXPECT_EQ(world.stored_food(), 70);
}

TEST(WorldTest, StartPosition) {
  World world(5, 5);
  EXPECT_EQ(world.start_position(), (util::Cell{0, 0}));
  world.set_start_position(util::Cell{3, 4});
  EXPECT_EQ(world.start_position(), (util::Cell{3, 4}));
}

TEST(WorldTest, CheckInvariantsValid) {
  World world(3, 3, util::Cell{0, 0});
  world[util::Cell{1, 1}].terrain = TerrainType::Storage;
  EXPECT_TRUE(world.check_invariants());
}

TEST(WorldTest, CheckInvariantsInvalidCases) {
  World empty_world;
  EXPECT_FALSE(empty_world.check_invariants());
  World out_of_bounds_world(3, 3, util::Cell{5, 5});
  out_of_bounds_world[util::Cell{1, 1}].terrain = TerrainType::Storage;
  EXPECT_FALSE(out_of_bounds_world.check_invariants());
  World wall_spawn_world(3, 3, util::Cell{0, 0});
  wall_spawn_world[util::Cell{0, 0}].terrain = TerrainType::Wall;
  wall_spawn_world[util::Cell{1, 1}].terrain = TerrainType::Storage;
  EXPECT_FALSE(wall_spawn_world.check_invariants());
  World no_storage_world(3, 3, util::Cell{0, 0});
  EXPECT_FALSE(no_storage_world.check_invariants());
}

} // namespace colony::world