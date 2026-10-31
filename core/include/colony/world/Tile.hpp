#pragma once

#include <compare>
#include <cstdint>

namespace colony::world {
enum class TerrainType : uint8_t { Ground, Wall, Storage };

enum class ResourceType : uint8_t { None, Food, Wood };

struct Tile {
  TerrainType terrain{TerrainType::Ground};
  ResourceType resource{ResourceType::None};
  std::uint32_t resource_amount{0};

  [[nodiscard]] constexpr bool is_walkable() const noexcept {
    return terrain != TerrainType::Wall;
  }

  [[nodiscard]] constexpr bool has_resource() const noexcept {
    return resource != ResourceType::None && resource_amount > 0;
  }

  [[nodiscard]] constexpr bool operator==(const Tile &other) const noexcept {
    return terrain == other.terrain && resource == other.resource &&
           resource_amount == other.resource_amount;
  }

  [[nodiscard]] constexpr bool operator!=(const Tile &other) const noexcept {
    return !(*this == other);
  }

  [[nodiscard]] constexpr bool operator<(const Tile &other) const noexcept {
    return terrain < other.terrain ||
           (terrain == other.terrain && resource < other.resource) ||
           (terrain == other.terrain && resource == other.resource &&
            resource_amount < other.resource_amount);
  }

  [[nodiscard]] constexpr bool operator>(const Tile &other) const noexcept {
    return other < *this;
  }

  [[nodiscard]] constexpr bool operator<=(const Tile &other) const noexcept {
    return !(other < *this);
  }

  [[nodiscard]] constexpr bool operator>=(const Tile &other) const noexcept {
    return !(*this < other);
  }
};
} // namespace colony::world