#pragma once

#include <colony/util/Grid.hpp>
#include <colony/world/Tile.hpp>

#include <cstddef>
#include <cstdint>

namespace colony::world {
class World {
private:
  util::Cell start_position_{0, 0};
  util::Grid<Tile> grid_;
  std::uint32_t stored_food_{0};
  std::uint32_t stored_wood_{0};

public:
  World() = default;

  World(size_t width, size_t height, util::Cell start_position = {0, 0})
      : grid_(width, height, Tile{}), start_position_(start_position) {}

  [[nodiscard]] constexpr std::size_t width() const noexcept {
    return grid_.width();
  }

  [[nodiscard]] constexpr std::size_t height() const noexcept {
    return grid_.height();
  }
  [[nodiscard]] bool in_bounds(util::Cell cell) const noexcept {
    return grid_.in_bounds(cell);
  }

  [[nodiscard]] Tile &operator[](util::Cell cell) noexcept {
    return grid_[cell];
  }
  [[nodiscard]] const Tile &operator[](util::Cell cell) const noexcept {
    return grid_[cell];
  }

  [[nodiscard]] Tile &at(util::Cell cell) { return grid_.at(cell); }

  [[nodiscard]] const Tile &at(util::Cell cell) const { return grid_.at(cell); }

  [[nodiscard]] constexpr util::Cell start_position() const noexcept {
    return start_position_;
  }

  [[nodiscard]] std::uint32_t stored_food() const noexcept {
    return stored_food_;
  }
  [[nodiscard]] std::uint32_t stored_wood() const noexcept {
    return stored_wood_;
  }

  void add_stored_food(std::uint32_t amount) noexcept {
    stored_food_ += amount;
  }
  void add_stored_wood(std::uint32_t amount) noexcept {
    stored_wood_ += amount;
  }

  void set_start_position(util::Cell cell) noexcept { start_position_ = cell; }

  [[nodiscard]] bool check_invariants() const noexcept {
    if (grid_.width() == 0 || grid_.height() == 0) {
      return false;
    }

    if (!grid_.in_bounds(start_position_)) {
      return false;
    }

    if (!grid_[start_position_].is_walkable()) {
      return false;
    }

    bool has_storage = false;
    for (std::size_t y = 0; y < grid_.height(); y++) {
      for (std::size_t x = 0; x < grid_.width(); x++) {
        if (grid_[util::Cell{static_cast<std::int32_t>(x),
                             static_cast<std::int32_t>(y)}]
                .terrain == TerrainType::Storage) {
          has_storage = true;
          break;
        }
      }
      if (has_storage)
        break;
    }
    return has_storage;
  }
};
} // namespace colony::world