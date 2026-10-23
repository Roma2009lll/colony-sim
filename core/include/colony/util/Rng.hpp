#pragma once

#include <cmath>
#include <cstdint>
#include <random>

namespace colony::util {
class Rng {
private:
  std::mt19937_64 engine_;

public:
  Rng() = default;

  explicit Rng(std::uint64_t seed) : engine_(seed) {}

  static Rng for_agent(std::uint64_t seed, std::uint64_t tick,
                       std::uint64_t agent_id) noexcept {
    std::uint64_t combined_seed = seed;
    combined_seed ^= tick + 0x9e3779b97f4a7c15ULL + (combined_seed << 6) +
                     (combined_seed >> 2);
    combined_seed ^= agent_id + 0x9e3779b97f4a7c15ULL + (combined_seed << 6) +
                     (combined_seed >> 2);
    return Rng(combined_seed);
  }

  [[nodiscard]] std::uint64_t next_u64() { return engine_(); }

  [[nodiscard]] std::int32_t range(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(engine_);
  }

  [[nodiscard]] double uniform() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(engine_);
  }

  [[nodiscard]] bool chance(double probability) {
    if (std::isnan(probability) || probability <= 0.0) {
      return false;
    }
    if (probability >= 1.0) {
      return true;
    }
    return uniform() < probability;
  }
};

} // namespace colony::util