#include <colony/world/MapLoader.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <variant>

namespace colony::world {
namespace {

// Тимчасовий файл у системному каталозі; видаляється в деструкторі.
class TempFile {
public:
  TempFile(const std::string &name, const std::string &content)
      : path_(std::filesystem::temp_directory_path() /
              ("colony_test_" + name)) {
    std::ofstream out(path_, std::ios::binary);
    out << content;
  }
  ~TempFile() {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }
  TempFile(const TempFile &) = delete;
  TempFile &operator=(const TempFile &) = delete;

  [[nodiscard]] std::string path() const { return path_.string(); }

private:
  std::filesystem::path path_;
};

std::string map_path(const std::string &file) {
  return (std::filesystem::path(COLONY_TEST_DATA_DIR) / "maps" / file)
      .string();
}

} // namespace

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
  const TempFile file("valid_map.txt", "###\n#@S\n###");
  auto result = MapLoader::load_from_file(file.path());
  ASSERT_TRUE(std::holds_alternative<World>(result));
  const auto &world = std::get<World>(result);
  EXPECT_EQ(world.width(), 3);
  EXPECT_EQ(world.height(), 3);
}

TEST(MapLoaderTest, LoadFromStringDoesNotModifyInputAndAcceptsLiterals) {
  const std::string map_data = "###\r\n#@S\r\n###\r\n";
  const std::string original = map_data;
  EXPECT_TRUE(std::holds_alternative<World>(MapLoader::load_from_string(map_data)));
  EXPECT_EQ(map_data, original);
  EXPECT_TRUE(std::holds_alternative<World>(
      MapLoader::load_from_string("###\n#@S\n###")));
}

TEST(MapLoaderTest, LoadFromStringSkipsUtf8Bom) {
  auto result = MapLoader::load_from_string("\xEF\xBB\xBF###\n#@S\n###");
  ASSERT_TRUE(std::holds_alternative<World>(result));
  EXPECT_EQ(std::get<World>(result).width(), 3u);
}

TEST(MapLoaderTest, LoadFromStringOnlyBomIsEmptyMap) {
  auto result = MapLoader::load_from_string("\xEF\xBB\xBF");
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::EmptyMap);
}

TEST(MapLoaderTest, LoadFromStringIgnoresTrailingBlankLines) {
  auto result = MapLoader::load_from_string("###\n#@S\n###\n\n\r\n\n");
  ASSERT_TRUE(std::holds_alternative<World>(result));
  EXPECT_EQ(std::get<World>(result).height(), 3u);
}

TEST(MapLoaderTest, LoadFromStringBlankLinesOnlyIsEmptyMap) {
  auto result = MapLoader::load_from_string("\n\r\n\n");
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::EmptyMap);
}

TEST(MapLoaderTest, LoadFromStringBlankLineInsideMapIsUnevenRows) {
  auto result = MapLoader::load_from_string("###\n\n#@S\n###");
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::UnevenRows);
}

TEST(MapLoaderTest, LoadFromStringLeadingBlankLineIsUnevenRows) {
  auto result = MapLoader::load_from_string("\n###\n#@S\n###");
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::UnevenRows);
}

TEST(MapLoaderTest, LoadFromStringStrayCarriageReturnIsUnknownSymbol) {
  auto result = MapLoader::load_from_string("#\r#\n#@S\n###");
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::UnknownSymbol);
}

TEST(MapLoaderTest, LoadFromFileDirectoryIsError) {
  auto result =
      MapLoader::load_from_file(std::filesystem::temp_directory_path().string());
  ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
  EXPECT_EQ(std::get<MapParseError>(result), MapParseError::FileNotFound);
}

TEST(MapLoaderTest, LoadFromFileWithWindowsNewlines) {
  const TempFile file("crlf_map.txt", "###\r\n#@S\r\n###\r\n");
  auto result = MapLoader::load_from_file(file.path());
  ASSERT_TRUE(std::holds_alternative<World>(result));
  EXPECT_EQ(std::get<World>(result).width(), 3u);
  EXPECT_EQ(std::get<World>(result).height(), 3u);
}

TEST(MapLoaderFilesTest, ValidMapFilesLoadAndSatisfyInvariants) {
  struct Case {
    const char *file;
    std::size_t width;
    std::size_t height;
  };
  const Case cases[] = {{"valid_small.txt", 7, 5},
                        {"valid_corridor.txt", 10, 5},
                        {"valid_colony.txt", 32, 16}};
  for (const Case &c : cases) {
    SCOPED_TRACE(c.file);
    auto result = MapLoader::load_from_file(map_path(c.file));
    ASSERT_TRUE(std::holds_alternative<World>(result));
    const auto &world = std::get<World>(result);
    EXPECT_EQ(world.width(), c.width);
    EXPECT_EQ(world.height(), c.height);
    EXPECT_TRUE(world.check_invariants());
  }
}

TEST(MapLoaderFilesTest, InvalidMapFilesReturnExpectedError) {
  struct Case {
    const char *file;
    MapParseError error;
  };
  const Case cases[] = {
      {"invalid_uneven.txt", MapParseError::UnevenRows},
      {"invalid_no_spawn.txt", MapParseError::MissingSpawn},
      {"invalid_no_storage.txt", MapParseError::InvariantViolation},
      {"invalid_unknown_symbol.txt", MapParseError::UnknownSymbol}};
  for (const Case &c : cases) {
    SCOPED_TRACE(c.file);
    auto result = MapLoader::load_from_file(map_path(c.file));
    ASSERT_TRUE(std::holds_alternative<MapParseError>(result));
    EXPECT_EQ(std::get<MapParseError>(result), c.error);
  }
}

} // namespace colony::world