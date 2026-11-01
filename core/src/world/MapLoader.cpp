#include <colony/world/MapLoader.hpp>

#include <filesystem>
#include <fstream>
#include <system_error>

std::string_view
colony::world::MapLoader::to_string(colony::world::MapParseError error) {
  switch (error) {
  case colony::world::MapParseError::FileNotFound:
    return "FileNotFound";
  case colony::world::MapParseError::EmptyMap:
    return "EmptyMap";
  case colony::world::MapParseError::UnevenRows:
    return "UnevenRows";
  case colony::world::MapParseError::UnknownSymbol:
    return "UnknownSymbol";
  case colony::world::MapParseError::MissingSpawn:
    return "MissingSpawn";
  case colony::world::MapParseError::MultipleSpawns:
    return "MultipleSpawns";
  case colony::world::MapParseError::InvariantViolation:
    return "InvariantViolation";
  }
  return "UnknownError";
}

std::variant<colony::world::World, colony::world::MapParseError>
colony::world::MapLoader::load_from_file(const std::string &filepath) {
  std::ifstream fp(filepath, std::ios::binary);
  if (!fp.is_open()) {
    return colony::world::MapParseError::FileNotFound;
  }
  std::error_code ec;
  auto file_size = std::filesystem::file_size(filepath, ec);
  if (ec) {
    return MapParseError::FileNotFound;
  }
  std::string content;
  content.resize(file_size);
  if (!fp.read(content.data(), static_cast<std::streamsize>(file_size))) {
    return MapParseError::FileNotFound;
  }
  return colony::world::MapLoader::load_from_string(content);
}

void colony::world::MapLoader::split_lines(
    std::string_view content, std::vector<std::string_view> &lines) {
  while (!content.empty()) {
    const std::size_t newline = content.find('\n');
    std::string_view line = content.substr(0, newline);
    content.remove_prefix(newline == std::string_view::npos ? content.size()
                                                            : newline + 1);
    // CRLF: прибираємо лише '\r' перед '\n'. Випадковий '\r' усередині рядка
    // лишається й стане помилкою UnknownSymbol.
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    lines.push_back(line);
  }
}

std::optional<colony::world::MapParseError>
colony::world::MapLoader::parse_symbol(char ch, World &world,
                                       colony::util::Cell &cell,
                                       std::uint32_t &spawn_count) {
  switch (ch) {
  case '#':
    world[cell].terrain = TerrainType::Wall;
    break;
  case '.':
    world[cell].terrain = TerrainType::Ground;
    break;
  case 'S':
    world[cell].terrain = TerrainType::Storage;
    break;
  case 'F':
    world[cell].terrain = TerrainType::Ground;
    world[cell].resource = ResourceType::Food;
    world[cell].resource_amount = 10;
    break;
  case 'T':
    world[cell].terrain = TerrainType::Ground;
    world[cell].resource = ResourceType::Wood;
    world[cell].resource_amount = 10;
    break;
  case '@':
    world[cell].terrain = TerrainType::Ground;
    world.set_start_position(cell);
    ++spawn_count;
    break;
  default:
    return colony::world::MapParseError::UnknownSymbol;
  }
  return std::nullopt;
}

std::variant<colony::world::World, colony::world::MapParseError>
colony::world::MapLoader::validate_world(const World &world,
                                         std::uint32_t spawn_count) {
  if (spawn_count == 0) {
    return colony::world::MapParseError::MissingSpawn;
  }
  if (spawn_count > 1) {
    return colony::world::MapParseError::MultipleSpawns;
  }
  if (!world.check_invariants()) {
    return colony::world::MapParseError::InvariantViolation;
  }

  return world;
}

std::variant<colony::world::World, colony::world::MapParseError>
colony::world::MapLoader::load_from_string(std::string_view content) {
  constexpr std::string_view utf8_bom = "\xEF\xBB\xBF";
  if (content.starts_with(utf8_bom)) {
    content.remove_prefix(utf8_bom.size());
  }
  std::vector<std::string_view> lines;
  split_lines(content, lines);
  // Порожні рядки в кінці файлу ігноруємо; порожній рядок усередині карти
  // лишається рядком нульової довжини й дає UnevenRows.
  while (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  if (lines.empty()) {
    return colony::world::MapParseError::EmptyMap;
  }
  const std::size_t height = lines.size();
  const std::size_t width = lines.front().size();
  if (width == 0) {
    return colony::world::MapParseError::UnevenRows;
  }
  colony::world::World world(width, height);
  std::uint32_t spawn_count = 0;
  for (std::size_t y = 0; y < height; ++y) {
    if (lines[y].size() != width) {
      return colony::world::MapParseError::UnevenRows;
    }
    for (std::size_t x = 0; x < width; ++x) {
      char ch = lines[y][x];
      util::Cell cell{static_cast<std::int32_t>(x),
                      static_cast<std::int32_t>(y)};
      auto res = parse_symbol(ch, world, cell, spawn_count);
      if (res.has_value()) {
        return res.value();
      }
    }
  }
  return validate_world(world, spawn_count);
}