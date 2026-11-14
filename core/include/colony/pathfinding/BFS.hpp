#pragma once

#include "colony/pathfinding/GraphConcept.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace colony::pathfinding {
class BfsPathFinder {
private:
  std::vector<NodeId> queue_{};
  std::vector<NodeId> came_from_{};
  std::vector<std::uint32_t> visited_gen_{};
  std::vector<NodeId> path_buffer_{};
  std::uint32_t current_gen_{0};

  constexpr void prepare(std::size_t count) {
    queue_.clear();
    path_buffer_.clear();
    if (visited_gen_.size() < count) {
      visited_gen_.resize(count, 0);
      came_from_.resize(count, k_invalid_node);
    }
    if (current_gen_ == std::numeric_limits<std::uint32_t>::max()) {
      std::fill(visited_gen_.begin(), visited_gen_.end(), 0);
      current_gen_ = 1;
    } else {
      current_gen_++;
    }
  }

public:
  template <GraphLike G>
  [[nodiscard]] PathView find_path(const G &graph, NodeId start, NodeId goal) {
    const std::size_t count = graph.node_count();
    if (static_cast<std::size_t>(start) >= count ||
        static_cast<std::size_t>(goal) >= count) {
      return PathView{};
    }
    if (!graph.is_walkable(start) || !graph.is_walkable(goal)) {
      return PathView{};
    }
    prepare(count);
    if (start == goal) {
      path_buffer_.push_back(start);
      return PathView{path_buffer_, Cost{0}, 0};
    }
    visited_gen_[start] = current_gen_;
    came_from_[start] = k_invalid_node;
    queue_.push_back(start);
    std::size_t head = 0;
    std::size_t expanded = 0;
    bool found = false;
    while (head < queue_.size()) {
      const NodeId u = queue_[head++];
      ++expanded;
      if (u == goal) {
        found = true;
        break;
      }
      graph.for_each_neighbor(u, [&](NodeId v, Cost) {
        if (visited_gen_[v] != current_gen_) {
          visited_gen_[v] = current_gen_;
          came_from_[v] = u;
          queue_.push_back(v);
        }
      });
    }
    if (!found) {
      return PathView{};
    }
    for (NodeId curr = goal; curr != k_invalid_node; curr = came_from_[curr]) {
      path_buffer_.push_back(curr);
    }
    std::reverse(path_buffer_.begin(), path_buffer_.end());
    return PathView{path_buffer_, static_cast<Cost>(path_buffer_.size() - 1),
                    expanded};
  }
};
} // namespace colony::pathfinding