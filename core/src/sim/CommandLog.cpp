#include <colony/sim/CommandLog.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <istream>
#include <optional>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <variant>

namespace colony::sim {
namespace {
[[nodiscard]] constexpr std::string_view cut_comments(std::string_view text) {
  size_t pos = text.find('#');
  if (pos == std::string_view::npos) {
    return text;
  }
  text.remove_prefix(pos);
  return text;
}

[[nodiscard]] constexpr std::string_view next_token(std::string_view text) {
  size_t start = text.find_first_not_of(" \t\r");
  if (start == std::string_view::npos) {
    text = {};
    return text;
  }
  text.remove_prefix(start);
  size_t end = text.find(" \t\r");
  std::string_view word = text.substr(0, end);
  if (end == std::string_view::npos) {
    text = {};
  } else {
    text.remove_prefix(end);
  }
  return word;
}

std::optional<std::uint32_t> to_uint32(std::string_view text) {
  std::uint32_t value = 0;
  auto [ptr, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec == std::errc{} && ptr == text.data() + text.size()) {
    return value;
  }
  return std::nullopt;
}

} // namespace

void CommandLog::sort() const {
  if (!is_sorted_) {
    std::sort(entries_.begin(), entries_.end(),
              [](const auto &a, const auto &b) { return a.tick_ < b.tick_; });
    is_sorted_ = true;
  }
}

void CommandLog::add(std::uint64_t tick, Command command) {
  entries_.emplace_back(tick, command);
}

[[nodiscard]] std::span<const CommandEntry>
CommandLog::commands_for_tick(std::uint64_t tick) const noexcept {
  CommandLog::sort();
  auto [first, last] = std::equal_range(
      entries_.begin(), entries_.end(), tick, [](const auto &a, const auto &b) {
        if constexpr (std::is_same_v<std::decay_t<decltype(a)>, CommandEntry>) {
          return a.tick_ < b;
        } else {
          return a < b.tick_;
        }
      });
  return std::span<const CommandEntry>(first, last);
}

void CommandLog::write_to(std::ostream &out) const {
  for (const auto &entry : entries_) {
    out << entry.tick_ << ' ';
    std::visit(
        [&out](const auto &cmd) {
          using T = std::decay_t<decltype(cmd)>;
          if constexpr (std::is_same_v<T, MarkDirective>) {
            out << "DIRECTIVE " << cmd.cell_.x << ' ' << cmd.cell_.y;
          } else if constexpr (std::is_same_v<T, CancelTask>) {
            out << "CANCEL " << cmd.task_id_.value();
          } else if constexpr (std::is_same_v<T, SetBalance>) {
            out << "BALANCE " << cmd.food_weight_;
          }
        },
        entry.command_);
    out << '\n';
  }
}

std::string CommandLog::to_string() const {
  std::ostringstream oss;
  write_to(oss);
  return oss.str();
}

bool CommandLog::save_to_file(const std::filesystem::path &path) const {
  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }
  write_to(file);
  return file.good();
}

[[nodiscard]] static CommandLogParseResult from_stream(std::istream &in) {}

[[nodiscard]] static CommandLogParseResult from_string(std::string_view text) {}

[[nodiscard]] static CommandLogParseResult
load_from_file(const std::filesystem::path &path) {
    
}
} // namespace colony::sim