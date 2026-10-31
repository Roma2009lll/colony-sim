#include <colony/world/Tile.hpp>
#include <gtest/gtest.h>

namespace colony::world {

TEST(TileTest, DefaultValues) {
  Tile tile;
  EXPECT_EQ(tile.terrain, TerrainType::Ground);
  EXPECT_EQ(tile.resource, ResourceType::None);
  EXPECT_EQ(tile.resource_amount, 0);
  EXPECT_TRUE(tile.is_walkable());
  EXPECT_FALSE(tile.has_resource());
}

TEST(TileTest, IsWalkable) {
  Tile ground{TerrainType::Ground, ResourceType::None, 0};
  Tile storage{TerrainType::Storage, ResourceType::None, 0};
  Tile wall{TerrainType::Wall, ResourceType::None, 0};

  EXPECT_TRUE(ground.is_walkable());
  EXPECT_TRUE(storage.is_walkable());
  EXPECT_FALSE(wall.is_walkable());
}

TEST(TileTest, HasResource) {
  Tile no_resource{TerrainType::Ground, ResourceType::None, 0};
  Tile food_zero{TerrainType::Ground, ResourceType::Food, 0};
  Tile food_present{TerrainType::Ground, ResourceType::Food, 10};
  Tile wood_present{TerrainType::Ground, ResourceType::Wood, 5};

  EXPECT_FALSE(no_resource.has_resource());
  EXPECT_FALSE(food_zero.has_resource());
  EXPECT_TRUE(food_present.has_resource());
  EXPECT_TRUE(wood_present.has_resource());
}

TEST(TileTest, EqualityAndInequality) {
  Tile t1{TerrainType::Ground, ResourceType::Food, 10};
  Tile t2{TerrainType::Ground, ResourceType::Food, 10};
  Tile t3{TerrainType::Wall, ResourceType::Food, 10};
  Tile t4{TerrainType::Ground, ResourceType::Wood, 10};
  Tile t5{TerrainType::Ground, ResourceType::Food, 5};

  EXPECT_EQ(t1, t2);
  EXPECT_NE(t1, t3);
  EXPECT_NE(t1, t4);
  EXPECT_NE(t1, t5);
}

TEST(TileTest, OrderingOperators) {
  Tile ground_none{TerrainType::Ground, ResourceType::None, 0};
  Tile ground_food_5{TerrainType::Ground, ResourceType::Food, 5};
  Tile ground_food_10{TerrainType::Ground, ResourceType::Food, 10};
  Tile wall_none{TerrainType::Wall, ResourceType::None, 0};
  EXPECT_LT(ground_none, wall_none);
  EXPECT_LE(ground_none, wall_none);
  EXPECT_LT(ground_none, ground_food_5);
  EXPECT_LT(ground_food_5, ground_food_10);
  EXPECT_GT(wall_none, ground_none);
  EXPECT_GE(wall_none, ground_none);
}

} // namespace colony::world