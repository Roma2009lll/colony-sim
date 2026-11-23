#include <colony/sim/StateHash.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace colony::sim {
namespace {
class Fnv1aHasher {
public:
  static constexpr std::uint64_t offset_basis = 0xcbf29ce484222325ull;
  static constexpr std::uint64_t prime = 0x100000001b3ull;

  constexpr Fnv1aHasher() noexcept = default;

  constexpr Fnv1aHasher &update(std::span<const std::uint8_t> data) noexcept {
    for (std::uint8_t byte : data) {
      hash_ ^= byte;
      hash_ *= prime;
    }
    return *this;
  }

  constexpr Fnv1aHasher &update(bool value) noexcept {
    std::uint8_t byte = value ? 0x01u : 0x00u;
    return update(std::span<const std::uint8_t>(&byte, 1));
  }

  template <std::integral T>
    requires(!std::same_as<std::remove_cv_t<T>, bool>)
  constexpr Fnv1aHasher &update(T value) noexcept {
    using UnsignedT = std::make_unsigned_t<T>;
    const auto uval = static_cast<UnsignedT>(value);
    std::array<std::uint8_t, sizeof(T)> bytes{};
    for (std::size_t i = 0; i < sizeof(T); i++) {
      bytes[i] = static_cast<std::uint8_t>((uval >> (i * 8)) & 0xFFu);
    }
    return update(bytes);
  }

  template <typename T>
    requires std::is_enum_v<T>
  constexpr Fnv1aHasher &update(T value) noexcept {
    return update(static_cast<std::underlying_type_t<T>>(value));
  }

  template <std::floating_point T>
  constexpr Fnv1aHasher &update(T value) noexcept {
    if (value == T{0}) {
      value = T{0};
    } else if (value != value) {
      value = std::numeric_limits<T>::quiet_NaN();
    }
    if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
      return update(std::bit_cast<std::uint32_t>(value));
    } else if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
      return update(std::bit_cast<std::uint64_t>(value));
    } else {
      static_assert(sizeof(T) == 4 || sizeof(T) == 8,
                    "Unsupported floating-point size");
    }
  }

  constexpr Fnv1aHasher &update(std::string_view str) noexcept {
    update(static_cast<std::uint64_t>(str.size()));
    for (char c : str) {
      hash_ ^= static_cast<std::uint8_t>(c);
      hash_ *= prime;
    }
    return *this;
  }

  template <typename T>
  constexpr Fnv1aHasher &update(const std::optional<T> &opt) noexcept {
    update(opt.has_value());
    if (opt.has_value()) {
      update(*opt);
    }
    return *this;
  }

  constexpr Fnv1aHasher &update(util::Cell cell) noexcept {
    return update(cell.x).update(cell.y);
  }

  template <typename Tag>
  constexpr Fnv1aHasher &update(util::Id<Tag> id) noexcept {
    update(id.is_valid());
    if (id.is_valid()) {
      update(id.value());
    }
    return *this;
  }

  template <typename T> constexpr Fnv1aHasher &update(T *) = delete;

  [[nodiscard]] constexpr std::uint64_t digest() const noexcept {
    return hash_;
  }

private:
  std::uint64_t hash_ = offset_basis;
};
} // namespace

constexpr StateHash
compute_state_hash(std::uint64_t tick, const world::World &world,
                   std::span<const agents::Agent> agents,
                   const tasks::TaskBoard &task_board,
                   const tasks::PriorityPolicy &priority_policy) {
  Fnv1aHasher hasher;
  hasher.update(tick);
  const auto width = world.width();
  const auto height = world.height();
  hasher.update(width)
      .update(height)
      .update(world.start_position())
      .update(world.stored_food())
      .update(world.stored_wood());
  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      const util::Cell cell{static_cast<decltype(util::Cell::x)>(x),
                            static_cast<decltype(util::Cell::y)>(y)};
      const auto &tile = world[cell];
      hasher.update(tile.terrain)
          .update(tile.resource)
          .update(tile.resource_amount);
    }
  }
  std::vector<const agents::Agent *> alive_agents;
  alive_agents.reserve(agents.size());
  for (const auto &agent : agents) {
    if (agent.is_alive()) {
      alive_agents.push_back(&agent);
    }
  }
  std::sort(alive_agents.begin(), alive_agents.end(),
            [](const agents::Agent *a, const agents::Agent *b) {
              return a->id() < b->id();
            });
  hasher.update(static_cast<std::uint64_t>(alive_agents.size()));
  for (const agents::Agent *agent : alive_agents) {
    const auto &needs = agent->needs();
    const auto &inv = agent->get_inventory();
    hasher.update(agent->id())
        .update(agent->position())
        .update(needs.hunger)
        .update(needs.health)
        .update(agent->get_intent())
        .update(agent->get_target_cell())
        .update(inv.resource)
        .update(inv.resource_amount)
        .update(agent->claimed_task_id());
  }
  const auto all_tasks = task_board.task_board();
  std::vector<const tasks::Task *> sorted_tasks;
  sorted_tasks.reserve(all_tasks.size());
  for (const auto &task : all_tasks) {
    sorted_tasks.push_back(&task);
  }
  std::sort(sorted_tasks.begin(), sorted_tasks.end(),
            [](const tasks::Task *a, const tasks::Task *b) {
              return a->id.value() < b->id.value();
            });
  hasher.update(static_cast<std::uint64_t>(sorted_tasks.size()));
  for (const tasks::Task *task : sorted_tasks) {
    hasher.update(task->id)
        .update(task->target_cell)
        .update(task->resource)
        .update(task->priority)
        .update(task->state)
        .update(task->assignee);
  }
  hasher.update(priority_policy.food_weight())
      .update(priority_policy.wood_weight());
  return hasher.digest();
}
} // namespace colony::sim