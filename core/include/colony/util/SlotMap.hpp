#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace colony::util {

static constexpr std::uint32_t k_invalid_index =
    std::numeric_limits<std::uint32_t>::max();

// dense_index_ == k_invalid_index означає, що слот не містить елемента
// (вільний або виведений з обігу).
struct Slot {
  std::uint32_t dense_index_{k_invalid_index};
  std::uint32_t generation_{0};
  std::uint32_t next_free_{k_invalid_index};
};

struct Handle {
  std::uint32_t index_{k_invalid_index};
  std::uint32_t generation_{0};
};

// T зберігається в std::vector і при видаленні переміщується на місце
// видаленого елемента, тому потрібні move-конструювання та move-присвоєння.
// Перевиділення vector усередині emplace переносить наявні елементи: якщо їх
// move-конструктор може кинути виняток і копіювання неможливе, вже перенесені
// елементи лишаються «порожніми». Тому move-конструктор має бути noexcept, або
// T має копіюватися (тоді vector копіює замість переміщення).
// bool виключено: std::vector<bool> не має data() і не віддає T&.
template <typename T>
concept SlotMapValue =
    std::movable<T> && !std::same_as<T, bool> &&
    (std::is_nothrow_move_constructible_v<T> ||
     std::is_copy_constructible_v<T>);

// SlotMap<T> - щільне сховище зі стабільними дескрипторами (Handle).
//
// Дескриптор = (індекс слота, покоління). Видалення збільшує покоління слота,
// тому застарілий дескриптор виявляється (is_valid / find) навіть після
// повторного використання слота. Слот, чиє покоління досягло максимуму,
// виводиться з обігу, тож лічильник ніколи не переповнюється.
//
// Інвалідація:
//  * emplace / insert можуть перевиділити пам'ять: усі вказівники, посилання
//    та span, отримані з find / operator[] / data(), стають недійсними.
//  * erase переносить ОСТАННІЙ елемент на місце видаленого (swap-and-pop):
//    недійсними стають вказівники на видалений і на останній елемент, а
//    порядок щільної ітерації змінюється. Цей порядок повністю визначається
//    послідовністю операцій. Дескриптори решти елементів лишаються дійсними.
//  * Видаляти під час ітерації по data() заборонено: елемент з кінця займає
//    місце видаленого, і лічильник циклу його пропустить. Збирайте дескриптори
//    потрібних елементів і видаляйте їх після циклу.
//
// Винятки (для T, що задовольняє SlotMapValue):
//  * emplace має строгу гарантію: якщо виняток кинув конструктор T або
//    виділення пам'яті, контейнер і список вільних слотів не змінюються.
//  * erase є noexcept рівно тоді, коли move-присвоєння T є noexcept. Якщо воно
//    все ж кине, службові індекси лишаються узгодженими: елемент не видалено,
//    усі дескриптори дійсні. Але move-присвоєння могло встигнути змінити обидва
//    об'єкти (той, що видаляється, і останній), тож їхні значення після
//    винятку невизначені.
template <SlotMapValue T> class SlotMap {
private:
  std::vector<T> dense_;
  std::vector<Slot> slots_;
  std::vector<std::uint32_t> dense_to_sparse_;
  std::uint32_t free_head_{k_invalid_index};

  friend struct SlotMapTestAccess;

  // Після move стан джерела має бути коректним порожнім контейнером, зокрема
  // free_head_ не повинен вказувати на слот, якого вже немає.
  constexpr void reset() noexcept {
    dense_.clear();
    slots_.clear();
    dense_to_sparse_.clear();
    free_head_ = k_invalid_index;
  }

  // Гарантує місце ще для одного елемента, щоб подальший push_back не кидав.
  // capacity() * 2 не переповнюється: capacity() <= max_size() <= PTRDIFF_MAX,
  // а надмірний запит reserve сам кидає std::length_error.
  template <typename U>
  static constexpr void reserve_one_more(std::vector<U> &vec) {
    if (vec.size() == vec.capacity()) {
      vec.reserve(vec.capacity() == 0 ? 8 : vec.capacity() * 2);
    }
  }

public:
  constexpr SlotMap() = default;
  constexpr SlotMap(const SlotMap &) = default;
  constexpr SlotMap &operator=(const SlotMap &) = default;

  constexpr SlotMap(SlotMap &&other) noexcept
      : dense_(std::move(other.dense_)), slots_(std::move(other.slots_)),
        dense_to_sparse_(std::move(other.dense_to_sparse_)),
        free_head_(other.free_head_) {
    other.reset();
  }

  constexpr SlotMap &operator=(SlotMap &&other) noexcept {
    if (this != &other) {
      dense_ = std::move(other.dense_);
      slots_ = std::move(other.slots_);
      dense_to_sparse_ = std::move(other.dense_to_sparse_);
      free_head_ = other.free_head_;
      other.reset();
    }
    return *this;
  }

  constexpr ~SlotMap() = default;

  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr Handle emplace(Args &&...args) {
    const bool reuse = free_head_ != k_invalid_index;
    if (!reuse && slots_.size() >= k_invalid_index) {
      throw std::length_error("SlotMap: slot index space exhausted");
    }
    // Усе, що може кинути bad_alloc, виконуємо до побудови T і до зміни
    // будь-якого стану. Після emplace_back далі нічого не кидає.
    reserve_one_more(dense_to_sparse_);
    if (!reuse) {
      reserve_one_more(slots_);
    }
    dense_.emplace_back(std::forward<Args>(args)...);

    std::uint32_t slot_index = free_head_;
    if (reuse) {
      free_head_ = slots_[slot_index].next_free_;
    } else {
      slot_index = static_cast<std::uint32_t>(slots_.size());
      slots_.emplace_back();
    }
    Slot &slot = slots_[slot_index];
    slot.dense_index_ = static_cast<std::uint32_t>(dense_.size() - 1);
    slot.next_free_ = k_invalid_index;
    dense_to_sparse_.push_back(slot_index);
    return Handle{.index_ = slot_index, .generation_ = slot.generation_};
  }

  constexpr Handle insert(T value) { return emplace(std::move(value)); }

  constexpr bool erase(Handle handle) noexcept(
      std::is_nothrow_move_assignable_v<T>) {
    if (!is_valid(handle)) {
      return false;
    }
    Slot &slot = slots_[handle.index_];
    const std::uint32_t del_index = slot.dense_index_;
    const auto last_el_index = static_cast<std::uint32_t>(dense_.size() - 1);
    if (del_index != last_el_index) {
      // Єдине місце, що може кинути виняток. Службові індекси ще не змінено,
      // тож структура лишається узгодженою, але значення обох елементів після
      // винятку невизначені (див. коментар до класу).
      dense_[del_index] = std::move(dense_[last_el_index]);
      const std::uint32_t last_slot_index = dense_to_sparse_[last_el_index];
      dense_to_sparse_[del_index] = last_slot_index;
      slots_[last_slot_index].dense_index_ = del_index;
    }
    dense_.pop_back();
    dense_to_sparse_.pop_back();
    slot.dense_index_ = k_invalid_index;
    if (slot.generation_ != std::numeric_limits<std::uint32_t>::max()) {
      slot.generation_++;
      slot.next_free_ = free_head_;
      free_head_ = handle.index_;
    }
    // Інакше покоління вичерпано: слот назавжди виведено з обігу, щоб
    // застарілий дескриптор не став знову дійсним після переповнення.
    return true;
  }

  [[nodiscard]] constexpr bool is_valid(Handle handle) const noexcept {
    return handle.index_ < slots_.size() &&
           slots_[handle.index_].generation_ == handle.generation_ &&
           slots_[handle.index_].dense_index_ != k_invalid_index;
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

  // Дескриптор має бути дійсним (перевіряється assert-ом у Debug).
  [[nodiscard]] constexpr T &operator[](Handle handle) {
    assert(is_valid(handle));
    return dense_[slots_[handle.index_].dense_index_];
  }

  [[nodiscard]] constexpr const T &operator[](Handle handle) const {
    assert(is_valid(handle));
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
