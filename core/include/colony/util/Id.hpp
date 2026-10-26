#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <compare>

namespace colony::util {

template <typename Tag, typename Underlying = std::uint32_t> class Id {

public:
  constexpr Id() noexcept = default;

  explicit constexpr Id(Underlying value) noexcept { this->value_ = value; }

  static constexpr Id invalid() noexcept { return Id(); }

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return value_ != invalid_value();
  }

  [[nodiscard]] constexpr Underlying value() const noexcept { return value_; }

  auto operator<=>(const Id &) const = default;

private:
  static constexpr Underlying invalid_value() noexcept {
    return std::numeric_limits<Underlying>::max();
  }

  Underlying value_ = invalid_value();
};
} // namespace colony::util

namespace std {
template <typename Tag, typename Underlying>
struct hash<colony::util::Id<Tag, Underlying>> {
  std::size_t
  operator()(const colony::util::Id<Tag, Underlying> id) const noexcept {
    return std::hash<Underlying>{}(id.value());
  }
};
} // namespace std