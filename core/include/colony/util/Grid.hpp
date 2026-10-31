#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace colony::util {

struct Cell {
  std::int32_t x{0};
  std::int32_t y{0};
  [[nodiscard]] constexpr bool operator==(const Cell &other) const noexcept {
    return x == other.x && y == other.y;
  }

  [[nodiscard]] constexpr bool operator!=(const Cell &other) const noexcept {
    return !(*this == other);
  }

  [[nodiscard]] constexpr bool operator<(const Cell &other) const noexcept {
    return x < other.x || (x == other.x && y < other.y);
  }

  [[nodiscard]] constexpr bool operator<=(const Cell &other) const noexcept {
    return !(other < *this);
  }

  [[nodiscard]] constexpr bool operator>(const Cell &other) const noexcept {
    return other < *this;
  }

  [[nodiscard]] constexpr bool operator>=(const Cell &other) const noexcept {
    return !(*this < other);
  }

  [[nodiscard]] constexpr Cell operator+(const Cell &other) const noexcept {
    return {x + other.x, y + other.y};
  }

  [[nodiscard]] constexpr Cell operator-(const Cell &other) const noexcept {
    return {x - other.x, y - other.y};
  }
};

template <typename T> class Grid {
public:
  Grid() = default;

  Grid(std::size_t width, std::size_t height, const T &default_value = T{})
      : data_(make_storage(checked_size(width, height))), width_{width},
        height_{height} {
    std::fill(begin(), end(), default_value);
  }

  Grid(const Grid &other)
      : data_(make_storage(other.size())), width_{other.width_},
        height_{other.height_} {
    std::copy(other.begin(), other.end(), begin());
  }

  Grid &operator=(const Grid &other) {
    if (this != &other) {
      Grid copy(other);
      *this = std::move(copy);
    }
    return *this;
  }

  Grid(Grid &&other) noexcept
      : data_(std::move(other.data_)),
        width_(std::exchange(other.width_, std::size_t{0})),
        height_(std::exchange(other.height_, std::size_t{0})) {}

  Grid &operator=(Grid &&other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      width_ = std::exchange(other.width_, std::size_t{0});
      height_ = std::exchange(other.height_, std::size_t{0});
    }
    return *this;
  }

  ~Grid() = default;

  [[nodiscard]] constexpr std::size_t width() const noexcept { return width_; }

  [[nodiscard]] constexpr std::size_t height() const noexcept {
    return height_;
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept {
    return width_ * height_;
  }

  [[nodiscard]] bool in_bounds(Cell cell) const noexcept {
    return cell.x >= 0 && static_cast<std::size_t>(cell.x) < width_ &&
           cell.y >= 0 && static_cast<std::size_t>(cell.y) < height_;
  }

  [[nodiscard]] T &operator[](Cell cell) noexcept {
    return data_[to_index(cell)];
  }

  [[nodiscard]] const T &operator[](Cell cell) const noexcept {
    return data_[to_index(cell)];
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

  [[nodiscard]] T *begin() noexcept { return data_.get(); }
  [[nodiscard]] T *end() noexcept { return data_.get() + size(); }
  [[nodiscard]] const T *begin() const noexcept { return data_.get(); }
  [[nodiscard]] const T *end() const noexcept { return data_.get() + size(); }

private:
  std::unique_ptr<T[]> data_;
  std::size_t width_{0};
  std::size_t height_{0};

  [[nodiscard]] static std::size_t checked_size(std::size_t width,
                                                std::size_t height) {
    if (width != 0 &&
        height > std::numeric_limits<std::size_t>::max() / width) {
      throw std::length_error("Grid: width * height overflows size_t");
    }
    return width * height;
  }

  [[nodiscard]] static std::unique_ptr<T[]> make_storage(std::size_t count) {
    return std::make_unique<T[]>(count);
  }

  [[nodiscard]] std::size_t to_index(Cell cell) const noexcept {
    assert(in_bounds(cell));
    return static_cast<std::size_t>(cell.y) * width_ +
           static_cast<std::size_t>(cell.x);
  }
};

} // namespace colony::util

namespace std {
template <> struct hash<colony::util::Cell> {
  std::size_t operator()(const colony::util::Cell &cell) const noexcept {
    const std::size_t h1 = std::hash<std::int32_t>{}(cell.x);
    const std::size_t h2 = std::hash<std::int32_t>{}(cell.y);
    return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
  }
};
} // namespace std