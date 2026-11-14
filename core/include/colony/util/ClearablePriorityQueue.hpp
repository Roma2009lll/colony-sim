#pragma once

#include <cstddef>
#include <functional>
#include <queue>
#include <vector>

namespace colony::util {

template <typename T, typename Container = std::vector<T>,
          typename Compare = std::less<typename Container::value_type>>
class ClearablePriorityQueue
    : public std::priority_queue<T, Container, Compare> {
public:
  using Base = std::priority_queue<T, Container, Compare>;
  using Base::Base;

  void clear() noexcept { this->c.clear(); }

  [[nodiscard]] constexpr std::size_t capacity() const noexcept {
    return this->c.capacity();
  }
};

} // namespace colony::util