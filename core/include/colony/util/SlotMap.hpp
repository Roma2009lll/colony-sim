#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace colony::util {
static constexpr std::uint32_t k_invalid_index =
    std::numeric_limits<std::uint32_t>::max();

struct Slot {
  std::uint32_t dense_index_;
  std::uint32_t generation_;
  std::uint32_t next_free_;
};

struct Handle {
  std::uint32_t index_;
  std::uint32_t generation_;
};
template <typename T> class SlotMap {
private:
  std::vector<T> dense_;
  std::vector<Slot> slots_;
  std::vector<std::uint32_t> dense_to_sparse_;
  std::uint32_t free_head_{k_invalid_index};

public:
  template <typename... Args> Handle constexpr emplace(Args &&...args) {
    std::uint32_t slot_index = -1;
    if (free_head_ != k_invalid_index) {
      slot_index = free_head_;
      free_head_ = slots_[slot_index].next_free_;
    } else {
      slot_index = std::static_cast<std::uint32_t>(slots_.size());
      slots_.emplace_back(Slot{
          .dense_index_ = 0, .generation_ = 0, .next_free_ = k_invalid_index});
    }
    dense_.emplace_back(std::forward<Args>(args)...);
    const auto dense_index = std::static_cast<std::uint32_t>(dense_.size() - 1);
    slots_[slot_index].dense_index_ = dense_index;
    dense_to_sparse_.emplace_back(slot_index);
    return Handle{.index_ = slot_index,
                  .generation_ = slots_[slot_index].generation_};
  }

  Handle constexpr insert(T value) { return emplace(std::move(value)); }
};
} // namespace colony::util