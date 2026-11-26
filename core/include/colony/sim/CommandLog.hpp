#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <colony/sim/Command.hpp>

namespace colony::sim {

struct CommandEntry {
  std::uint64_t tick_{0};
  Command command_{};

  constexpr bool operator==(const CommandEntry &) const noexcept = default;
};

struct CommandLogParseResult;

class CommandLog {
private:
  mutable std::vector<CommandEntry> entries_{};
  mutable bool is_sorted_ = false;
  void sort() const; 
 

public:
  CommandLog() = default;

  void add(std::uint64_t tick, Command command);

  [[nodiscard]] std::span<const CommandEntry>
  commands_for_tick(std::uint64_t tick) const noexcept;

  [[nodiscard]] std::span<const CommandEntry> entries() const noexcept {
    return entries_;
  }

  [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }

  [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

  void clear() noexcept { entries_.clear(); }
 
  void write_to(std::ostream &out) const;
  
  [[nodiscard]] std::string to_string() const;

  [[nodiscard]] bool save_to_file(const std::filesystem::path &path) const;

  [[nodiscard]] static CommandLogParseResult from_stream(std::istream &in);

  [[nodiscard]] static CommandLogParseResult from_string(std::string_view text);

  [[nodiscard]] static CommandLogParseResult
  load_from_file(const std::filesystem::path &path);

  bool operator==(const CommandLog &) const noexcept = default;
};

struct CommandLogParseResult {
  bool ok_{false};
  CommandLog log_{};
  std::size_t error_line_{0};
  std::string error_message_{};
};

} // namespace colony::sim