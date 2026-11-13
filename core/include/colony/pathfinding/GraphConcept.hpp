#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace colony::pathfinding {
using NodeId = std::uint32_t;
using Cost = std::uint32_t;

inline constexpr NodeId k_invalid_node = std::numeric_limits<NodeId>::max();
inline constexpr Cost k_invalid_cost = std::numeric_limits<Cost>::max();

struct PathView {
  std::span<const NodeId> path_{};
  Cost total_cost_{0};
  std::size_t expanded_nodes_{0};

  [[nodiscard]] constexpr bool found() const noexcept { return !path_.empty(); }
};

template <typename G>
concept GraphLike = requires(const G &graph, NodeId u, NodeId v) {
  { graph.node_count() } -> std::convertible_to<std::size_t>;

  { graph.is_walkable(u) } -> std::convertible_to<bool>;

  { graph.heuristic(u, v) } -> std::convertible_to<Cost>;

  graph.for_each_neighbor(u, [](NodeId, Cost) {});
};

} // namespace colony::pathfinding