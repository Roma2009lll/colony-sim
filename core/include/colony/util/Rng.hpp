#pragma once

#include <cmath>
#include <cstdint>
#include <random>
#include <utility>

namespace colony::util {
class Rng {
private:
  std::mt19937_64 engine_;

  [[nodiscard]] static std::uint64_t constexpr mix64(std::uint64_t x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }

public:
  Rng() = default;

  explicit Rng(std::uint64_t seed) : engine_(seed) {}

  [[nodiscard]] static Rng for_agent(std::uint64_t master_seed,
                                     std::uint64_t tick,
                                     std::uint64_t agent_id) noexcept {
    std::uint64_t h = master_seed;
    h ^= mix64(tick + 0x9e3779b97f4a7c15ULL);
    h ^= mix64(agent_id + 0xbf58476d1ce4e5b9ULL);
    return Rng(mix64(h));
  }

  [[nodiscard]] std::uint64_t next_u64() { return engine_(); }
  
  [[nodiscard]] std::int32_t range(int min, int max) {
    if (min > max)
      std::swap(min, max);
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