#include <colony/world/MapLoader.hpp>

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

void colony::world::MapLoader::remove_carriage_returns(std::string &content) {
  auto new_end = std::remove(content.begin(), content.end(), '\r');
  content.erase(new_end, content.end());
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
  const char *const start_ptr = content.data();
  const char *const end_ptr = start_ptr + content.size();
  const char *current_start = start_ptr;
  for (const char *p = start_ptr; p < end_ptr; p++) {
    if (*p == '\n') {
      size_t len = p - current_start;
      if (len > 0) {
        lines.emplace_back(current_start, len);
      }
      current_start = p + 1;
    }
  }
  if (current_start < end_ptr) {
    lines.emplace_back(current_start, end_ptr - current_start);
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
colony::world::MapLoader::load_from_string(std::string &content) {
  colony::world::MapLoader::remove_carriage_returns(content);
  if (content.empty()) {
    return colony::world::MapParseError::EmptyMap;
  }
  if (content.rfind("\xEF\xBB\xBF", 0) == 0) {
    content.erase(0, 3);
  }
  std::vector<std::string_view> lines;
  split_lines(content, lines);
  if (lines.empty()) {
    return colony::world::MapParseError::EmptyMap;
  }
  const std::size_t height = lines.size();
  const std::size_t width = lines[0].size();
  if (width == 0) {
    return colony::world::MapParseError::EmptyMap;
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