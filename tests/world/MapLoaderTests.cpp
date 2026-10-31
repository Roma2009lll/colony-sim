#include <colony/world/MapLoader.hpp>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

namespace colony::world {

TEST(MapLoaderTest, ToStringFormatting) {
  EXPECT_EQ(MapLoader::to_string(MapParseError::FileNotFound), "FileNotFound");
  EXPECT_EQ(MapLoader::to_string(MapParseError::EmptyMap), "EmptyMap");
  EXPECT_EQ(MapLoader::to_string(MapParseError::UnevenRows), "UnevenRows");
  EXPECT_EQ(MapLoader::to_string(MapParseError::UnknownSymbol),
            "UnknownSymbol");
  EXPECT_EQ(MapLoader::to_string(MapParseError::MissingSpawn), "MissingSpawn");
  EXPECT_EQ(MapLoader::to_string(MapParseError::MultipleSpawns),
            "MultipleSpawns");
  EXPECT_EQ(MapLoader::to_string(MapParseError::InvariantViolation),
            "InvariantViolation");
}

TEST(MapLoaderTest, LoadFromStringValidMap) {
  std::string map_data = "#####\n"
                         "#@.S#\n"
                         "#.F.#\n"
                         "#..T#\n"
                         "#####";
  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<World>(result));
  const auto &world = std::get<World>(result);
  EXPECT_EQ(world.width(), 5);
  EXPECT_EQ(world.height(), 5);
  EXPECT_EQ(world.start_position(), (util::Cell{1, 1}));
  EXPECT_EQ((world[util::Cell{1, 1}].terrain), TerrainType::Ground);
  EXPECT_EQ((world[util::Cell{3, 1}].terrain), TerrainType::Storage);
  EXPECT_EQ((world[util::Cell{2, 2}].resource), ResourceType::Food);
  EXPECT_EQ((world[util::Cell{2, 2}].resource_amount), 10);
  EXPECT_EQ((world[util::Cell{3, 3}].resource), ResourceType::Wood);
  EXPECT_EQ((world[util::Cell{3, 3}].resource_amount), 10);
}

TEST(MapLoaderTest, LoadFromStringWindowsNewlines) {
  std::string map_data = "###\r\n#@S\r\n###\r\n";
  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<World>(result));
  const auto &world = std::get<World>(result);
  EXPECT_EQ(world.width(), 3);
  EXPECT_EQ(world.height(), 3);
  EXPECT_EQ(world.start_position(), (util::Cell{1, 1}));
}

TEST(MapLoaderTest, LoadFromStringEmptyMap) {
  std::string empty_str = "";
  auto result = MapLoader::load_from_string(empty_str);
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::EmptyMap);
}

TEST(MapLoaderTest, LoadFromStringUnevenRows) {
  std::string map_data = "#####\n"
                         "#@S#\n"
                         "#####";
  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::UnevenRows);
}

TEST(MapLoaderTest, LoadFromStringUnknownSymbol) {
  std::string map_data = "###\n"
                         "#@X\n"
                         "#S#";
  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::UnknownSymbol);
}

TEST(MapLoaderTest, LoadFromStringMissingSpawn) {
  std::string map_data = "###\n"
                         "#.S\n"
                         "###";

  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::MissingSpawn);
}

TEST(MapLoaderTest, LoadFromStringMultipleSpawns) {
  std::string map_data = "###\n"
                         "#@@\n"
                         "#S#";
  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::MultipleSpawns);
}

TEST(MapLoaderTest, LoadFromStringInvariantViolationNoStorage) {
  std::string map_data = "###\n"
                         "#@.\n"
                         "###";
  auto result = MapLoader::load_from_string(map_data);
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::InvariantViolation);
}

TEST(MapLoaderTest, LoadFromFileNotFound) {
  auto result = MapLoader::load_from_file("non_existent_map_file.txt");
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::FileNotFound);
}

TEST(MapLoaderTest, LoadFromFileValid) {
  const std::string temp_filename = "temp_test_map.txt";
  {
    std::ofstream out(temp_filename, std::ios::binary);
    out << "###\n#@S\n###";
  }
  auto result = MapLoader::load_from_file(temp_filename);
  std::filesystem::remove(temp_filename);
  ASSERT_TRUE(std::holds_alternative<World>(result));
  const auto &world = std::get<World>(result);
  EXPECT_EQ(world.width(), 3);
  EXPECT_EQ(world.height(), 3);
}

} // namespace colony::world