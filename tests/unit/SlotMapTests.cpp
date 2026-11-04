#include <colony/util/Rng.hpp>
#include <colony/util/SlotMap.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace colony::util {

// Доступ до приватних полів для тесту вичерпання покоління (див. friend у SlotMap).
struct SlotMapTestAccess {
  template <typename T>
  static void set_generation(SlotMap<T> &map, std::uint32_t slot,
                             std::uint32_t generation) {
    map.slots_[slot].generation_ = generation;
  }
  template <typename T>
  static std::uint32_t generation(const SlotMap<T> &map, std::uint32_t slot) {
    return map.slots_[slot].generation_;
  }
};

} // namespace colony::util

namespace colony::util::testing {

TEST(SlotMapTest, DefaultStateAndEmplace) {
  SlotMap<std::string> map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  Handle h1 = map.emplace("Alpha");
  Handle h2 = map.insert("Beta");
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.is_valid(h1));
  EXPECT_TRUE(map.is_valid(h2));
  ASSERT_NE(map.find(h1), nullptr);
  ASSERT_NE(map.find(h2), nullptr);
  EXPECT_EQ(*map.find(h1), "Alpha");
  EXPECT_EQ(*map.find(h2), "Beta");
  EXPECT_EQ(map[h1], "Alpha");
  EXPECT_EQ(map[h2], "Beta");
}

TEST(SlotMapTest, EraseAndStaleHandle) {
  SlotMap<int> map;
  Handle h1 = map.emplace(100);
  Handle h2 = map.emplace(200);
  EXPECT_TRUE(map.erase(h1));
  EXPECT_EQ(map.size(), 1);
  EXPECT_FALSE(map.is_valid(h1));
  EXPECT_EQ(map.find(h1), nullptr);
  EXPECT_TRUE(map.is_valid(h2));
  ASSERT_NE(map.find(h2), nullptr);
  EXPECT_EQ(map[h2], 200);
}

TEST(SlotMapTest, SwapAndPopIntegrity) {
  SlotMap<int> map;
  Handle h0 = map.emplace(10);
  Handle h1 = map.emplace(20); 
  Handle h2 = map.emplace(30); 
  EXPECT_EQ(map.size(), 3);
  EXPECT_TRUE(map.erase(h1));
  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.is_valid(h0));
  EXPECT_TRUE(map.is_valid(h2));
  EXPECT_EQ(map[h0], 10);
  EXPECT_EQ(map[h2], 30);
  auto span = map.data();
  ASSERT_EQ(span.size(), 2);
  EXPECT_EQ(span[0], 10);
  EXPECT_EQ(span[1], 30);
}

TEST(SlotMapTest, SlotReuseAndGenerationIncrement) {
  SlotMap<std::string> map;
  Handle old_h = map.emplace("OldValue");
  const std::uint32_t original_index = old_h.index_;
  const std::uint32_t original_gen = old_h.generation_;
  EXPECT_TRUE(map.erase(old_h));
  Handle new_h = map.emplace("NewValue");
  EXPECT_EQ(new_h.index_, original_index);
  EXPECT_EQ(new_h.generation_, original_gen + 1);
  EXPECT_FALSE(map.is_valid(old_h));
  EXPECT_EQ(map.find(old_h), nullptr);
  EXPECT_TRUE(map.is_valid(new_h));
  EXPECT_EQ(map[new_h], "NewValue");
}

TEST(SlotMapTest, DataSpanMutationAndConstAccess) {
  SlotMap<int> map;
  map.emplace(1);
  map.emplace(2);
  map.emplace(3);
  for (auto& val : map.data()) {
    val *= 10;
  }
  const auto& const_map = map;
  EXPECT_EQ(const_map.size(), 3);
  auto const_span = const_map.data();
  ASSERT_EQ(const_span.size(), 3);
  EXPECT_EQ(const_span[0], 10);
  EXPECT_EQ(const_span[1], 20);
  EXPECT_EQ(const_span[2], 30);
}

TEST(SlotMapTest, InvalidAndOutOfBoundsHandles) {
  SlotMap<int> map;
  Handle default_handle{};
  Handle out_of_bounds_handle{.index_ = 999, .generation_ = 0};
  EXPECT_FALSE(map.is_valid(default_handle));
  EXPECT_FALSE(map.is_valid(out_of_bounds_handle));
  EXPECT_EQ(map.find(default_handle), nullptr);
  EXPECT_EQ(map.find(out_of_bounds_handle), nullptr);
  EXPECT_FALSE(map.erase(default_handle));
  EXPECT_FALSE(map.erase(out_of_bounds_handle));
}


// ---------------------------------------------------------------------------
// Обмеження на T (requires / concept)
// ---------------------------------------------------------------------------

struct NonMovable {
  NonMovable() = default;
  NonMovable(NonMovable &&) = delete;
  NonMovable &operator=(NonMovable &&) = delete;
};

struct ConstMember {
  const int value{1};
};

struct ThrowingMove {
  int value{0};
  ThrowingMove() = default;
  explicit ThrowingMove(int v) : value(v) {}
  ThrowingMove(ThrowingMove &&) = default;
  ThrowingMove &operator=(ThrowingMove &&other) noexcept(false) {
    if (other.value == 13) {
      throw std::runtime_error("move assignment");
    }
    value = other.value;
    return *this;
  }
};

template <typename T>
concept CanMakeSlotMap = requires { typename SlotMap<T>; };

static_assert(SlotMapValue<int>);
static_assert(SlotMapValue<std::string>);
static_assert(SlotMapValue<std::unique_ptr<int>>);
static_assert(SlotMapValue<ThrowingMove>);
static_assert(!SlotMapValue<bool>);
static_assert(!SlotMapValue<int &>);
static_assert(!SlotMapValue<NonMovable>);
static_assert(!SlotMapValue<ConstMember>);
static_assert(CanMakeSlotMap<int>);
static_assert(!CanMakeSlotMap<bool>);
static_assert(!CanMakeSlotMap<NonMovable>);
static_assert(!CanMakeSlotMap<ConstMember>);

static_assert(noexcept(std::declval<SlotMap<int> &>().erase(Handle{})));
static_assert(
    !noexcept(std::declval<SlotMap<ThrowingMove> &>().erase(Handle{})));
static_assert(std::is_nothrow_move_constructible_v<SlotMap<std::string>>);
static_assert(std::is_nothrow_move_assignable_v<SlotMap<std::string>>);

TEST(SlotMapTest, MoveOnlyElementType) {
  SlotMap<std::unique_ptr<int>> map;
  const Handle first = map.insert(std::make_unique<int>(5));
  const Handle second = map.emplace(std::make_unique<int>(6));
  EXPECT_TRUE(map.erase(first));
  ASSERT_NE(map.find(second), nullptr);
  EXPECT_EQ(**map.find(second), 6);
}

// ---------------------------------------------------------------------------
// Копіювання та переміщення самого контейнера
// ---------------------------------------------------------------------------

TEST(SlotMapTest, CopyIsIndependentAndKeepsHandles) {
  SlotMap<std::string> original;
  const Handle h1 = original.emplace("x");
  const Handle h2 = original.emplace("y");
  SlotMap<std::string> copy = original;
  EXPECT_TRUE(copy.is_valid(h1));
  EXPECT_EQ(copy[h2], "y");
  EXPECT_TRUE(original.erase(h1));
  EXPECT_FALSE(original.is_valid(h1));
  EXPECT_TRUE(copy.is_valid(h1));
  EXPECT_EQ(copy.size(), 2u);
}

TEST(SlotMapTest, MovedFromMapIsEmptyAndReusable) {
  SlotMap<int> source;
  const Handle h0 = source.emplace(1);
  const Handle h1 = source.emplace(2);
  ASSERT_TRUE(source.erase(h1)); // у джерела непорожній список вільних слотів

  SlotMap<int> target = std::move(source);
  EXPECT_TRUE(target.is_valid(h0));
  EXPECT_FALSE(target.is_valid(h1));
  EXPECT_TRUE(source.empty());

  const Handle again = source.emplace(7); // раніше: розіменування free_head_
  EXPECT_TRUE(source.is_valid(again));
  EXPECT_EQ(source[again], 7);
  EXPECT_EQ(source.size(), 1u);
  EXPECT_EQ(target.size(), 1u);
}

TEST(SlotMapTest, MoveAssignmentLeavesSourceReusable) {
  SlotMap<int> source;
  const Handle a = source.emplace(1);
  source.emplace(2);
  ASSERT_TRUE(source.erase(a));

  SlotMap<int> target;
  target.emplace(100);
  target = std::move(source);
  EXPECT_EQ(target.size(), 1u);
  EXPECT_TRUE(source.empty());

  const Handle again = source.emplace(9);
  EXPECT_EQ(source[again], 9);
  const Handle reused = target.emplace(3); // вільний слот переїхав разом з даними
  EXPECT_EQ(reused.index_, a.index_);
  EXPECT_EQ(reused.generation_, a.generation_ + 1);

  SlotMap<int> &alias = target;
  target = std::move(alias); // самоприсвоєння не руйнує контейнер
  EXPECT_EQ(target.size(), 2u);
}

// ---------------------------------------------------------------------------
// Покоління
// ---------------------------------------------------------------------------

TEST(SlotMapGenerationTest, HandleToFreeSlotWithMatchingGenerationIsInvalid) {
  SlotMap<int> map;
  const Handle h = map.emplace(5);
  ASSERT_TRUE(map.erase(h));
  // Те саме покоління, яке слот матиме в наступного власника (як після
  // переповнення лічильника): вільний слот не повинен вважатися зайнятим.
  const Handle forged{.index_ = h.index_, .generation_ = h.generation_ + 1};
  EXPECT_FALSE(map.is_valid(forged));
  EXPECT_EQ(map.find(forged), nullptr);
  EXPECT_FALSE(map.erase(forged));
}

TEST(SlotMapGenerationTest, ExhaustedGenerationRetiresSlot) {
  constexpr std::uint32_t max_generation =
      std::numeric_limits<std::uint32_t>::max();
  SlotMap<int> map;
  const Handle first = map.emplace(1);
  ASSERT_TRUE(map.erase(first));
  SlotMapTestAccess::set_generation(map, first.index_, max_generation);

  const Handle last_use = map.emplace(2);
  EXPECT_EQ(last_use.index_, first.index_);
  EXPECT_EQ(last_use.generation_, max_generation);
  ASSERT_TRUE(map.erase(last_use));

  EXPECT_FALSE(map.is_valid(last_use));
  EXPECT_EQ(SlotMapTestAccess::generation(map, last_use.index_),
            max_generation); // не обернулося до 0
  const Handle next = map.emplace(3);
  EXPECT_NE(next.index_, last_use.index_); // слот виведено з обігу
  EXPECT_FALSE(map.is_valid(last_use));
  EXPECT_FALSE(map.is_valid(Handle{.index_ = last_use.index_, .generation_ = 0}));
}

// ---------------------------------------------------------------------------
// Виняткова безпека
// ---------------------------------------------------------------------------

struct ThrowOnConstruct {
  static inline bool armed = false;
  int value;
  explicit ThrowOnConstruct(int v) : value(v) {
    if (armed) {
      throw std::runtime_error("constructor");
    }
  }
  ThrowOnConstruct(ThrowOnConstruct &&) noexcept = default;
  ThrowOnConstruct &operator=(ThrowOnConstruct &&) noexcept = default;
};

TEST(SlotMapExceptionTest, ThrowingConstructorKeepsFreeSlot) {
  SlotMap<ThrowOnConstruct> map;
  const Handle keep = map.emplace(1);
  const Handle gone = map.emplace(2);
  ASSERT_TRUE(map.erase(gone));

  ThrowOnConstruct::armed = true;
  EXPECT_THROW(map.emplace(3), std::runtime_error);
  ThrowOnConstruct::armed = false;

  EXPECT_EQ(map.size(), 1u);
  EXPECT_TRUE(map.is_valid(keep));
  const Handle again = map.emplace(4);
  EXPECT_EQ(again.index_, gone.index_); // слот не загублено
  EXPECT_EQ(again.generation_, gone.generation_ + 1);
}

TEST(SlotMapExceptionTest, ThrowingConstructorDoesNotCreateSlot) {
  SlotMap<ThrowOnConstruct> map;
  const Handle first = map.emplace(1);

  ThrowOnConstruct::armed = true;
  EXPECT_THROW(map.emplace(2), std::runtime_error);
  ThrowOnConstruct::armed = false;

  const Handle second = map.emplace(3);
  EXPECT_EQ(second.index_, first.index_ + 1);
  EXPECT_EQ(map.size(), 2u);
}

TEST(SlotMapExceptionTest, ThrowingMoveAssignmentInEraseKeepsMapConsistent) {
  SlotMap<ThrowingMove> map;
  const Handle first = map.emplace(1);
  const Handle second = map.emplace(13); // на його місце переносимо - кидає
  EXPECT_THROW(map.erase(first), std::runtime_error);
  EXPECT_EQ(map.size(), 2u);
  EXPECT_TRUE(map.is_valid(first));
  EXPECT_TRUE(map.is_valid(second));

  EXPECT_TRUE(map.erase(second)); // останній елемент: без переміщення
  EXPECT_EQ(map.size(), 1u);
  EXPECT_TRUE(map.is_valid(first));
}

// ---------------------------------------------------------------------------
// Тип-інструмент: лічильники копій / переміщень / деструкторів
// ---------------------------------------------------------------------------

struct Counters {
  int constructed{0};
  int copy_constructed{0};
  int move_constructed{0};
  int copy_assigned{0};
  int move_assigned{0};
  int destroyed{0};
  [[nodiscard]] int alive() const {
    return constructed + copy_constructed + move_constructed - destroyed;
  }
};

class Tracked {
public:
  static inline Counters counters{};
  static void reset() { counters = Counters{}; }

  int value;

  explicit Tracked(int v) : value(v) { ++counters.constructed; }
  Tracked(const Tracked &other) : value(other.value) {
    ++counters.copy_constructed;
  }
  Tracked(Tracked &&other) noexcept : value(other.value) {
    ++counters.move_constructed;
  }
  Tracked &operator=(const Tracked &other) {
    value = other.value;
    ++counters.copy_assigned;
    return *this;
  }
  Tracked &operator=(Tracked &&other) noexcept {
    value = other.value;
    ++counters.move_assigned;
    return *this;
  }
  ~Tracked() { ++counters.destroyed; }
};

TEST(SlotMapInstrumentedTest, EmplaceConstructsInPlace) {
  Tracked::reset();
  SlotMap<Tracked> map;
  map.emplace(1);
  EXPECT_EQ(Tracked::counters.constructed, 1);
  EXPECT_EQ(Tracked::counters.copy_constructed, 0);
  EXPECT_EQ(Tracked::counters.move_constructed, 0);
  EXPECT_EQ(Tracked::counters.destroyed, 0);
}

TEST(SlotMapInstrumentedTest, EraseInMiddleMovesLastIntoHole) {
  SlotMap<Tracked> map;
  const Handle first = map.emplace(1);
  map.emplace(2);
  map.emplace(3);
  Tracked::reset();

  ASSERT_TRUE(map.erase(first));
  EXPECT_EQ(Tracked::counters.move_assigned, 1);
  EXPECT_EQ(Tracked::counters.copy_assigned, 0);
  EXPECT_EQ(Tracked::counters.copy_constructed, 0);
  EXPECT_EQ(Tracked::counters.destroyed, 1);
  const auto dense = map.data();
  ASSERT_EQ(dense.size(), 2u);
  EXPECT_EQ(dense[0].value, 3);
  EXPECT_EQ(dense[1].value, 2);
}

TEST(SlotMapInstrumentedTest, EraseOfLastDoesNotMove) {
  SlotMap<Tracked> map;
  map.emplace(1);
  const Handle last = map.emplace(2);
  Tracked::reset();

  ASSERT_TRUE(map.erase(last));
  EXPECT_EQ(Tracked::counters.move_assigned, 0);
  EXPECT_EQ(Tracked::counters.move_constructed, 0);
  EXPECT_EQ(Tracked::counters.destroyed, 1);
}

TEST(SlotMapInstrumentedTest, NoCopiesAndNoLeaksOverManyOperations) {
  Tracked::reset();
  {
    SlotMap<Tracked> map;
    std::vector<Handle> handles;
    for (int i = 0; i < 200; ++i) {
      handles.push_back(map.emplace(i)); // кілька перевиділень vector
    }
    for (std::size_t i = 0; i < handles.size(); i += 2) {
      ASSERT_TRUE(map.erase(handles[i]));
    }
    for (int i = 0; i < 50; ++i) {
      map.insert(Tracked{i});
    }
    EXPECT_EQ(Tracked::counters.copy_constructed, 0);
    EXPECT_EQ(Tracked::counters.copy_assigned, 0);
    EXPECT_EQ(Tracked::counters.alive(), static_cast<int>(map.size()));
  }
  EXPECT_EQ(Tracked::counters.alive(), 0); // усі деструктори викликано
}

// ---------------------------------------------------------------------------
// Модельний тест: випадкові операції проти еталона на std::vector
// ---------------------------------------------------------------------------

struct ModelEntry {
  Handle handle;
  int value;
};

// Еталон повторює правило swap-and-pop на звичайному std::vector, тому порівняння
// перевіряє і набір елементів, і порядок щільної ітерації.
void run_model(std::uint64_t seed, int steps, std::vector<int> &final_order) {
  Rng rng(seed);
  SlotMap<int> map;
  std::vector<ModelEntry> model;
  std::vector<Handle> stale;
  int next_value = 0;

  const auto check_dense = [&] {
    const auto dense = map.data();
    ASSERT_EQ(dense.size(), model.size());
    std::unordered_set<std::uint32_t> live_slots;
    for (std::size_t i = 0; i < model.size(); ++i) {
      ASSERT_EQ(dense[i], model[i].value) << "seed " << seed << ", pos " << i;
      ASSERT_TRUE(map.is_valid(model[i].handle));
      ASSERT_EQ(map[model[i].handle], model[i].value);
      live_slots.insert(model[i].handle.index_);
    }
    ASSERT_EQ(live_slots.size(), model.size()) << "два живі елементи в одному слоті";
  };

  for (int step = 0; step < steps; ++step) {
    const int op = rng.range(0, 99);
    if (op < 45 || model.empty()) {
      const Handle h =
          rng.chance(0.5) ? map.emplace(next_value) : map.insert(next_value);
      model.push_back(ModelEntry{h, next_value});
      ++next_value;
    } else if (op < 75) {
      const auto i = static_cast<std::size_t>(
          rng.range(0, static_cast<int>(model.size()) - 1));
      const Handle h = model[i].handle;
      ASSERT_TRUE(map.erase(h));
      model[i] = model.back();
      model.pop_back();
      stale.push_back(h);
    } else if (op < 85) {
      if (!stale.empty()) {
        const auto i = static_cast<std::size_t>(
            rng.range(0, static_cast<int>(stale.size()) - 1));
        const Handle h = stale[i];
        ASSERT_FALSE(map.is_valid(h));
        ASSERT_EQ(map.find(h), nullptr);
        ASSERT_FALSE(map.erase(h));
      }
    } else {
      const auto i = static_cast<std::size_t>(
          rng.range(0, static_cast<int>(model.size()) - 1));
      const int *found = std::as_const(map).find(model[i].handle);
      ASSERT_NE(found, nullptr);
      ASSERT_EQ(*found, model[i].value);
    }
    ASSERT_EQ(map.size(), model.size());
    if (step % 64 == 0) {
      check_dense();
      if (::testing::Test::HasFatalFailure()) {
        return;
      }
    }
  }

  check_dense();
  if (::testing::Test::HasFatalFailure()) {
    return;
  }
  for (const Handle &h : stale) {
    ASSERT_FALSE(map.is_valid(h)) << "застарілий дескриптор знову дійсний";
  }
  final_order.assign(map.data().begin(), map.data().end());
}

TEST(SlotMapModelTest, RandomOperationsMatchReferenceModelOnManySeeds) {
  for (std::uint64_t seed = 1; seed <= 40; ++seed) {
    SCOPED_TRACE(::testing::Message() << "seed " << seed);
    std::vector<int> order;
    ASSERT_NO_FATAL_FAILURE(run_model(seed, 20000, order));
  }
}

TEST(SlotMapModelTest, DenseOrderIsDeterminedByOperationSequence) {
  for (std::uint64_t seed : {7ULL, 123ULL, 2026ULL}) {
    std::vector<int> first;
    std::vector<int> second;
    ASSERT_NO_FATAL_FAILURE(run_model(seed, 10000, first));
    ASSERT_NO_FATAL_FAILURE(run_model(seed, 10000, second));
    EXPECT_EQ(first, second) << "seed " << seed;
    EXPECT_FALSE(first.empty());
  }
}

} // namespace colony::util::testing
