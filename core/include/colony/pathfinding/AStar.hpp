#pragma once

#include "colony/pathfinding/GraphConcept.hpp"
#include "colony/util/ClearablePriorityQueue.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace colony::pathfinding {

struct QueueEntry {
  NodeId node_;
  Cost f_;
  Cost g_;
};

struct Comparator {
  bool operator()(const QueueEntry &a1, const QueueEntry &a2) const {
    if (a1.f_ != a2.f_) {
      return a1.f_ > a2.f_;
    }
    if (a1.g_ != a2.g_) {
      return a1.g_ < a2.g_;
    }
    return a1.node_ > a2.node_;
  }
};
class AStarPathFinder {
private:
  util::ClearablePriorityQueue<QueueEntry, std::vector<QueueEntry>, Comparator>
      p_queue_{};
  std::vector<NodeId> came_from_{};
  std::vector<std::uint32_t> visited_gen_{};
  std::vector<NodeId> path_buffer_{};
  std::uint32_t current_gen_{0};
  std::vector<Cost> g_score_{};

  void prepare(std::size_t count) {
    p_queue_.clear();
    path_buffer_.clear();
    if (visited_gen_.size() < count) {
      visited_gen_.resize(count, 0);
      came_from_.resize(count, k_invalid_node);
      g_score_.resize(count);
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
    if (start >= count || goal >= count) {
      return PathView{};
    }
    if (!graph.is_walkable(start) || !graph.is_walkable(goal)) {
      return PathView{};
    }
    if (start == goal) {
      path_buffer_.clear();
      path_buffer_.push_back(start);
      return PathView{
          .path_ = path_buffer_,
          .total_cost_ = 0,
          .expanded_nodes_ = 0,
      };
    }
    prepare(count);
    visited_gen_[start] = current_gen_;
    came_from_[start] = k_invalid_node;
    g_score_[start] = 0;
    const Cost start_h = graph.heuristic(start, goal);
    p_queue_.push(QueueEntry{start, start_h, 0});
    std::size_t expanded_nodes = 0;
    while (!p_queue_.empty()) {
      const QueueEntry current = p_queue_.top();
      p_queue_.pop();
      if (current.g_ > g_score_[current.node_]) {
        continue;
      }
      ++expanded_nodes;
      if (current.node_ == goal) {
        break;
      }
      graph.for_each_neighbor(
          current.node_, [&](NodeId neighbor, Cost step_cost) {
            if (neighbor >= count) {
              return;
            }
            const Cost new_g = current.g_ + step_cost;
            const bool is_unvisited = (visited_gen_[neighbor] != current_gen_);
            if (is_unvisited || new_g < g_score_[neighbor]) {
              visited_gen_[neighbor] = current_gen_;
              g_score_[neighbor] = new_g;
              came_from_[neighbor] = current.node_;
              const Cost f = new_g + graph.heuristic(neighbor, goal);
              p_queue_.push(QueueEntry{neighbor, f, new_g});
            }
          });
    }
    if (visited_gen_[goal] != current_gen_) {
      return PathView{};
    }
    for (NodeId curr = goal; curr != k_invalid_node; curr = came_from_[curr]) {
      path_buffer_.push_back(curr);
    }
    std::reverse(path_buffer_.begin(), path_buffer_.end());
    return PathView{
        .path_ = path_buffer_,
        .total_cost_ = g_score_[goal],
        .expanded_nodes_ = expanded_nodes,
    };
  }
};
} // namespace colony::pathfinding