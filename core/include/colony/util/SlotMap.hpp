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
  std::uint32_t dense_index_{0};
  std::uint32_t generation_{0};
  std::uint32_t next_free_{k_invalid_index};
};

struct Handle {
  std::uint32_t index_{k_invalid_index};
  std::uint32_t generation_{0};
};

template <typename T> class SlotMap {
private:
  std::vector<T> dense_;
  std::vector<Slot> slots_;
  std::vector<std::uint32_t> dense_to_sparse_;
  std::uint32_t free_head_{k_invalid_index};

public:
  template <typename... Args> Handle constexpr emplace(Args &&...args) {
    std::uint32_t slot_index = k_invalid_index;
    if (free_head_ != k_invalid_index) {
      slot_index = free_head_;
      free_head_ = slots_[slot_index].next_free_;
    } else {
      slot_index = static_cast<std::uint32_t>(slots_.size());
      slots_.emplace_back(Slot{
          .dense_index_ = 0, .generation_ = 0, .next_free_ = k_invalid_index});
    }
    dense_.emplace_back(std::forward<Args>(args)...);
    const auto dense_index = static_cast<std::uint32_t>(dense_.size() - 1);
    slots_[slot_index].dense_index_ = dense_index;
    dense_to_sparse_.emplace_back(slot_index);
    return Handle{.index_ = slot_index,
                  .generation_ = slots_[slot_index].generation_};
  }

  Handle constexpr insert(T value) { return emplace(std::move(value)); }

  bool constexpr erase(Handle handle) noexcept {
    if (!is_valid(handle)) {
      return false;
    }
    const std::uint32_t del_index = slots_[handle.index_].dense_index_;
    const std::uint32_t last_el_index =
        static_cast<std::uint32_t>(dense_.size() - 1);
    if (del_index != last_el_index) {
      dense_[del_index] = std::move(dense_[last_el_index]);
      const std::uint32_t last_slot_index = dense_to_sparse_[last_el_index];
      dense_to_sparse_[del_index] = last_slot_index;
      slots_[last_slot_index].dense_index_ = del_index;
    }
    dense_.pop_back();
    dense_to_sparse_.pop_back();
    slots_[handle.index_].generation_++;
    slots_[handle.index_].next_free_ = free_head_;
    free_head_ = handle.index_;
    return true;
  }

  [[nodiscard]] constexpr bool is_valid(Handle handle) const noexcept {
    return handle.index_ < slots_.size() &&
           slots_[handle.index_].generation_ == handle.generation_;
  }

  [[nodiscard]] constexpr T *find(Handle handle) noexcept {
    if (!is_valid(handle)) {
      return nullptr;
    }
    return &dense_[slots_[handle.index_].dense_index_];
  }

  [[nodiscard]] constexpr const T *find(Handle handle) const noexcept {
    if (!is_valid(handle)) {
      return nullptr;
    }
    return &dense_[slots_[handle.index_].dense_index_];
  }

  [[nodiscard]] constexpr T &operator[](Handle handle) {
    return dense_[slots_[handle.index_].dense_index_];
  }

  [[nodiscard]] constexpr const T &operator[](Handle handle) const {
    return dense_[slots_[handle.index_].dense_index_];
  }

  [[nodiscard]] constexpr std::span<T> data() noexcept {
    return std::span<T>{dense_.data(), dense_.size()};
  }

  [[nodiscard]] constexpr std::span<const T> data() const noexcept {
    return std::span<const T>{dense_.data(), dense_.size()};
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept {
    return dense_.size();
  }

  [[nodiscard]] constexpr bool empty() const noexcept { return dense_.empty(); }
};

} // namespace colony::util