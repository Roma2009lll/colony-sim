#pragma once

#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace colony::util {
struct Cell {
  std::int32_t x{0};
  std::int32_t y{0};
  bool operator==(const Cell &cell) const {
    return x == cell.x && y == cell.y;
  };

  bool operator<(const Cell &cell) const noexcept {
    return x < cell.x || (x == cell.x && y < cell.y);
  }

  constexpr Cell operator+(const Cell &other) const noexcept {
    return {x + other.x, y + other.y};
  }

  constexpr Cell operator-(const Cell &other) const noexcept {
    return {x - other.x, y - other.y};
  }
}; 

template <typename T> class Grid {
public:
  using StorageType =
      std::conditional_t<std::is_same_v<T, bool>, std::uint8_t, T>;

  constexpr Grid() = default;

  Grid(size_t width, size_t height, const T &default_value = T{})
      : width_{width}, height_{height},
        data_(width * height, static_cast<StorageType>(default_value)) {}

  Grid(Grid &&other) noexcept {
    data_ = std::move(other.data_);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
  }

  Grid &operator=(Grid &&other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      width_ = std::exchange(other.width_, 0);
      height_ = std::exchange(other.height_, 0);
    }
    return *this;
  }

  Grid(const Grid &) = default;
  Grid &operator=(const Grid &) = default;
  ~Grid() = default;

  [[nodiscard]] size_t constexpr width() const noexcept { return width_; }

  [[nodiscard]] size_t constexpr height() const noexcept { return height_; }

  [[nodiscard]] size_t constexpr size() const noexcept {
    return width_ * height_;
  }

  [[nodiscard]] bool in_bounds(Cell cell) const noexcept {
    return cell.x >= 0 && static_cast<std::size_t>(cell.x) < width_ &&
           cell.y >= 0 && static_cast<std::size_t>(cell.y) < height_;
  }

  [[nodiscard]] T &operator[](const Cell &cell) noexcept {
    return reinterpret_cast<T &>(data_[to_index(cell)]);
  }

  [[nodiscard]] const T &operator[](Cell cell) const noexcept {
    return reinterpret_cast<const T &>(data_[to_index(cell)]);
  }

  [[nodiscard]] T &operator()(std::int32_t x, std::int32_t y) noexcept {
    return (*this)[Cell{x, y}];
  }

  [[nodiscard]] const T &operator()(std::int32_t x,
                                    std::int32_t y) const noexcept {
    return (*this)[Cell{x, y}];
  }

  [[nodiscard]] T &at(Cell cell) {
    if (!in_bounds(cell)) {
      throw std::out_of_range("Grid::at - index out of bounds");
    }
    return (*this)[cell];
  }

  [[nodiscard]] const T &at(Cell cell) const {
    if (!in_bounds(cell)) {
      throw std::out_of_range("Grid::at - index out of bounds");
    }
    return (*this)[cell];
  }

  auto begin() noexcept { return reinterpret_cast<T *>(data_.data()); }
  auto end() noexcept {
    return reinterpret_cast<T *>(data_.data() + data_.size());
  }
  auto begin() const noexcept {
    return reinterpret_cast<const T *>(data_.data());
  }
  auto end() const noexcept {
    return reinterpret_cast<const T *>(data_.data() + data_.size());
  }

private:
  std::vector<StorageType> data_;
  size_t width_{0};
  size_t height_{0};

  [[nodiscard]] size_t to_index(const Cell &cell) const noexcept {
    assert(in_bounds(cell));
    return static_cast<size_t>(cell.y) * width_ + static_cast<size_t>(cell.x);
  }
};
} // namespace colony::util

namespace std {
template <> struct hash<colony::util::Cell> {
  std::size_t operator()(const colony::util::Cell &cell) const noexcept {
    size_t h1 = std::hash<std::int32_t>{}(cell.x);
    size_t h2 = std::hash<std::int32_t>{}(cell.y);
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
  }
};
} // namespace std