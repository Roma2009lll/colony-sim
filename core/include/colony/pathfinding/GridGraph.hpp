#pragma once

#include "colony/pathfinding/GraphConcept.hpp"
#include "colony/util/Grid.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace colony::pathfinding {
template <typename T, typename WalkablePred> class GridGraph {
private:
  const util::Grid<T> &grid_;
  WalkablePred pred_;

public:
  explicit GridGraph(const util::Grid<T> &grid, WalkablePred pred)
      : grid_(grid), pred_(pred) {}

  constexpr pathfinding::NodeId to_node(util::Cell cell) const {
    return static_cast<pathfinding::NodeId>(static_cast<std::size_t>(cell.y) *
                                                grid_.width() +
                                            static_cast<std::size_t>(cell.x));
  }

  constexpr util::Cell to_cell(NodeId node_id) const {
    // Порожня сітка не має вузлів; без цієї перевірки було б ділення на нуль.
    if (grid_.width() == 0) {
      return util::Cell{0, 0};
    }
    return util::Cell{static_cast<std::int32_t>(node_id % grid_.width()),
                      static_cast<std::int32_t>(node_id / grid_.width())};
  }

  constexpr std::size_t node_count() const { return grid_.size(); }

  constexpr bool is_walkable(NodeId u) const {
    if (static_cast<std::size_t>(u) >= this->node_count()) {
      return false;
    }
    return pred_(grid_[this->to_cell(u)]);
  }

  constexpr pathfinding::Cost heuristic(pathfinding::NodeId u,
                                        pathfinding::NodeId v) const {
    util::Cell cell0 = this->to_cell(u);
    util::Cell cell1 = this->to_cell(v);
    return static_cast<pathfinding::Cost>(std::abs(cell0.x - cell1.x) +
                                          std::abs(cell0.y - cell1.y));
  }

  template <typename CallBack>
  void for_each_neighbor(pathfinding::NodeId u, CallBack &&func) const {
    if (static_cast<std::size_t>(u) >= this->node_count()) {
      return;
    } 
    static constexpr std::array<util::Cell, 4> k_dirs{
        {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    const util::Cell current = this->to_cell(u);
    for (const auto &dir : k_dirs) {
      const util::Cell next = current + dir;
      if (grid_.in_bounds(next) && this->is_walkable(this->to_node(next))) {
        func(this->to_node(next), Cost{1});
      }
    }
  }
};
} // namespace colony::pathfinding