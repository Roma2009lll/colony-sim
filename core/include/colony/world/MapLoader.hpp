#pragma once

#include "World.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace colony::world {

enum class MapParseError {
  FileNotFound,
  EmptyMap,
  UnevenRows,
  UnknownSymbol,
  MissingSpawn,
  MultipleSpawns,
  InvariantViolation
};

class MapLoader {
private:
  static void remove_carriage_returns(std::string &content);

  static void split_lines(std::string_view content,
                          std::vector<std::string_view> &lines);

  static std::optional<MapParseError> parse_symbol(char ch, World &world,
                                                   colony::util::Cell &cell,
                                                   std::uint32_t &spawn_count);

  static std::variant<World, MapParseError>
  validate_world(const World &world, std::uint32_t spawn_count);

public:
  MapLoader() = delete;
  static std::variant<World, MapParseError>
  load_from_string(std::string &content);

  static std::variant<World, MapParseError>
  load_from_file(const std::string &filepath);

  static std::string_view to_string(MapParseError error);
};

} // namespace colony::world