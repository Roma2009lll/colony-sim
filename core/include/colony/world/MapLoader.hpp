#pragma once

#include <colony/world/World.hpp>

#include <cstdint>
#include <optional>
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
  static void split_lines(std::string_view content,
                          std::vector<std::string_view> &lines);

  static std::optional<MapParseError> parse_symbol(char ch, World &world,
                                                   colony::util::Cell &cell,
                                                   std::uint32_t &spawn_count);

  static std::variant<World, MapParseError>
  validate_world(const World &world, std::uint32_t spawn_count);

public:
  MapLoader() = delete;

  // Символи: '#' стіна, '.' земля, 'S' склад, 'F' їжа, 'T' дерево, '@' старт.
  // Кінці рядків: \n або \r\n; порожні рядки в кінці та UTF-8 BOM ігноруються.
  // Вхідний текст не змінюється.
  static std::variant<World, MapParseError>
  load_from_string(std::string_view content);

  static std::variant<World, MapParseError>
  load_from_file(const std::string &filepath);

  static std::string_view to_string(MapParseError error);
};

} // namespace colony::world