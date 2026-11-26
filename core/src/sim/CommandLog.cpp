#include <colony/sim/CommandLog.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <istream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace colony::sim {
namespace {

[[nodiscard]] constexpr std::string_view cut_comments(std::string_view text) {
  const std::size_t pos = text.find('#');
  if (pos == std::string_view::npos) {
    return text;
  }
  return text.substr(0, pos);
}

[[nodiscard]] constexpr std::string_view next_token(std::string_view &text) {
  constexpr std::string_view kWhitespace = " \t\r";
  const std::size_t start = text.find_first_not_of(kWhitespace);
  if (start == std::string_view::npos) {
    text = {};
    return {};
  }
  text.remove_prefix(start);
  const std::size_t end = text.find_first_of(kWhitespace);
  const std::string_view word = text.substr(0, end);
  if (end == std::string_view::npos) {
    text = {};
  } else {
    text.remove_prefix(end);
  }
  return word;
}

template <typename T>
[[nodiscard]] std::optional<T> parse_number(std::string_view text) {
  T value{};
  const auto [ptr, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  if constexpr (std::is_floating_point_v<T>) {
    if (!std::isfinite(value)) {
      return std::nullopt;
    }
  }
  return value;
}

} // namespace

void CommandLog::sort() const {
  if (!is_sorted_) {
    std::stable_sort(
        entries_.begin(), entries_.end(),
        [](const auto &a, const auto &b) { return a.tick_ < b.tick_; });
    is_sorted_ = true;
  }
}

void CommandLog::add(std::uint64_t tick, Command command) {
  entries_.emplace_back(tick, std::move(command));
  is_sorted_ = false; 
}

[[nodiscard]] std::span<const CommandEntry>
CommandLog::commands_for_tick(std::uint64_t tick) const noexcept {
  sort();
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

CommandLogParseResult CommandLog::from_stream(std::istream &in) {
  CommandLog log;
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(in, line)) {
    ++line_number;
    std::string_view view = cut_comments(line);
    const std::string_view tick_tok = next_token(view);
    if (tick_tok.empty()) {
      continue;
    }
    const auto tick = parse_number<std::uint64_t>(tick_tok);
    if (!tick) {
      return CommandLogParseResult{
          .ok_ = false,
          .error_line_ = line_number,
          .error_message_ = "Invalid tick",
      };
    }
    const std::string_view cmd_tok = next_token(view);
    if (cmd_tok.empty()) {
      return CommandLogParseResult{
          .ok_ = false,
          .error_line_ = line_number,
          .error_message_ = "Missing command",
      };
    }
    std::optional<Command> parsed_cmd;
    if (cmd_tok == "DIRECTIVE") {
      const std::string_view x_tok = next_token(view);
      const std::string_view y_tok = next_token(view);
      if (x_tok.empty() || y_tok.empty()) {
        return CommandLogParseResult{
            .ok_ = false,
            .error_line_ = line_number,
            .error_message_ = "DIRECTIVE requires x and y coordinates",
        };
      }
      const auto x = parse_number<std::uint32_t>(x_tok);
      const auto y = parse_number<std::uint32_t>(y_tok);
      if (!x || !y) {
        return CommandLogParseResult{
            .ok_ = false,
            .error_line_ = line_number,
            .error_message_ = "Invalid coordinates for DIRECTIVE",
        };
      }
      parsed_cmd = MarkDirective{{*x, *y}};
    } else if (cmd_tok == "CANCEL") {
      const std::string_view id_tok = next_token(view);
      if (id_tok.empty()) {
        return CommandLogParseResult{
            .ok_ = false,
            .error_line_ = line_number,
            .error_message_ = "CANCEL requires task_id",
        };
      }
      const auto id = parse_number<std::uint32_t>(id_tok);
      if (!id) {
        return CommandLogParseResult{
            .ok_ = false,
            .error_line_ = line_number,
            .error_message_ = "Invalid task_id for CANCEL",
        };
      }
      parsed_cmd = CancelTask{colony::tasks::TaskId{*id}};
    } else if (cmd_tok == "BALANCE") {
      const std::string_view weight_tok = next_token(view);
      if (weight_tok.empty()) {
        return CommandLogParseResult{
            .ok_ = false,
            .error_line_ = line_number,
            .error_message_ = "BALANCE requires food_weight",
        };
      }
      using WeightT = std::decay_t<decltype(SetBalance::food_weight_)>;
      const auto weight = parse_number<WeightT>(weight_tok);
      if (!weight) {
        return CommandLogParseResult{
            .ok_ = false,
            .error_line_ = line_number,
            .error_message_ = "Invalid food_weight for BALANCE",
        };
      }
      parsed_cmd = SetBalance{*weight};
    } else {
      return CommandLogParseResult{
          .ok_ = false,
          .error_line_ = line_number,
          .error_message_ = "Unknown command",
      };
    }
    if (!next_token(view).empty()) {
      return CommandLogParseResult{
          .ok_ = false,
          .error_line_ = line_number,
          .error_message_ = "Unexpected trailing tokens",
      };
    }
    log.entries_.emplace_back(*tick, std::move(*parsed_cmd));
  }

  if (in.bad()) {
    return CommandLogParseResult{
        .ok_ = false,
        .error_line_ = line_number,
        .error_message_ = "Stream read error",
    };
  }
  log.is_sorted_ = false;
  log.sort();
  return CommandLogParseResult{
      .ok_ = true,
      .log_ = std::move(log),
  };
}

CommandLogParseResult CommandLog::from_string(std::string_view text) {
  std::istringstream iss{std::string{text}};
  return from_stream(iss);
}

CommandLogParseResult
CommandLog::load_from_file(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return CommandLogParseResult{
        .ok_ = false,
        .error_line_ = 0,
        .error_message_ = "Failed to open file",
    };
  }
  return from_stream(file);
}

} // namespace colony::sim