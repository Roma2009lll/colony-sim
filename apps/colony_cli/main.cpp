#include <colony/agents/Agent.hpp>
#include <colony/sim/Command.hpp>
#include <colony/sim/SimConfig.hpp>
#include <colony/sim/Simulation.hpp>
#include <colony/world/MapLoader.hpp>
#include <colony/world/World.hpp>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct CliOptions {
  std::filesystem::path map_path{};
  std::optional<std::filesystem::path> commands_path{};
  std::uint64_t seed{0};
  std::uint64_t max_ticks{1000};
  bool show_help{false};
};

[[nodiscard]] std::optional<std::uint64_t> parse_uint64(std::string_view text) {
  std::uint64_t value = 0;
  const auto [ptr, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

void print_help(std::string_view program_name) {
  std::cout
      << "Usage: " << program_name << " --map <path> [options]\n\n"
      << "Options:\n"
      << "  --map <path>        Path to ASCII map file (required)\n"
      << "  --commands <path>   Path to commands scenario file\n"
      << "  --seed <uint64>     RNG seed (default: 0)\n"
      << "  --ticks <uint64>    Maximum simulation ticks (default: 1000)\n"
      << "  -h, --help          Show this help message\n";
}

[[nodiscard]] std::optional<CliOptions> parse_command_line(int argc,
                                                           char *argv[]) {
  if (argc <= 0 || argv == nullptr) {
    return std::nullopt;
  }
  CliOptions cli{};
  const std::span<char *> args(argv, static_cast<std::size_t>(argc));
  for (std::size_t i = 1; i < args.size(); ++i) {
    const std::string_view arg = args[i];
    if (arg == "-h" || arg == "--help") {
      cli.show_help = true;
      return cli;
    }
    if (arg == "--map" || arg == "--commands" || arg == "--seed" ||
        arg == "--ticks") {
      if (i + 1 >= args.size()) {
        std::cerr << "Error: missing value for argument '" << arg << "'\n";
        return std::nullopt;
      }
      const std::string_view value = args[++i];
      if (arg == "--map") {
        cli.map_path = value;
      } else if (arg == "--commands") {
        cli.commands_path = std::filesystem::path(value);
      } else if (arg == "--seed") {
        const auto parsed = parse_uint64(value);
        if (!parsed) {
          std::cerr << "Error: invalid value for --seed: '" << value << "'\n";
          return std::nullopt;
        }
        cli.seed = *parsed;
      } else if (arg == "--ticks") {
        const auto parsed = parse_uint64(value);
        if (!parsed) {
          std::cerr << "Error: invalid value for --ticks: '" << value << "'\n";
          return std::nullopt;
        }
        cli.max_ticks = *parsed;
      }
    } else {
      std::cerr << "Error: unknown argument '" << arg << "'\n";
      return std::nullopt;
    }
  }
  if (cli.map_path.empty()) {
    std::cerr << "Error: --map <path> is required\n";
    return std::nullopt;
  }
  return cli;
}

} // namespace

int main(int argc, char *argv[]) {
  const auto options = parse_command_line(argc, argv);
  if (!options) {
    return EXIT_FAILURE;
  }
  if (options->show_help) {
    const std::string_view prog =
        (argc > 0 && argv[0] != nullptr) ? argv[0] : "colony_cli";
    print_help(prog);
    return EXIT_SUCCESS;
  }
  auto world_load_res =
      colony::world::MapLoader::load_from_file(options->map_path.string());
  if (std::holds_alternative<colony::world::MapParseError>(world_load_res)) {
    const auto &err =
        std::get<colony::world::MapParseError>(world_load_res);
    std::cerr << "Error loading map '" << options->map_path.string()
              << "': " << colony::world::MapLoader::to_string(err) << '\n';
    return EXIT_FAILURE;
  }
  auto world = std::get<colony::world::World>(std::move(world_load_res));
  std::vector<colony::agents::Agent> agents;
  colony::sim::SimConfig config{};
  agents.reserve(config.initial_agents_);
  for (std::uint32_t i = 1; i <= config.initial_agents_; i++) {
    agents.emplace_back(i, world.start_position());
  }
  colony::sim::CommandLog command_log{};
  if (options->commands_path) {
    auto cmd_res =
        colony::sim::CommandLog::load_from_file(*options->commands_path);
    if (!cmd_res.ok_) {
      std::cerr << "Error loading commands '"
                << options->commands_path->string() << "' at line "
                << cmd_res.error_line_ << ": " << cmd_res.error_message_
                << '\n';
      return EXIT_FAILURE;
    }
    command_log = std::move(cmd_res.log_);
  }
  colony::sim::Simulation sim(std::move(world), std::move(agents),
                              options->seed, std::move(command_log));
  sim.run(options->max_ticks);
  const auto &stats = sim.stats();
  const auto hash = sim.state_hash();
  std::cout << std::format("Simulation finished at tick {} (seed: {})\n"
                           "Colonists alive: {} (born: {}, died: {})\n"
                           "Collected total: food = {}, wood = {}\n"
                           "Tasks completed: {}\n"
                           "Directives: accepted = {}, rejected = {}\n"
                           "Final state hash: {:016x}\n",
                           sim.tick(), sim.seed(), sim.agents().size(),
                           stats.colonists_born, stats.colonists_died,
                           stats.food_collected, stats.wood_collected,
                           stats.tasks_completed, stats.directives_accepted,
                           stats.directives_rejected, hash);
  return EXIT_SUCCESS;
}
