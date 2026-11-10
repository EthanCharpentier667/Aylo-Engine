/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-export: exports a game, the folder a player runs, with only the plugins and backends its project.luau needs
// (docs/release/export.md). In the release zip it is in `tools/`, and ships the zip's `runtime/`.
//   aylo-export <game> <output> [--name <name>] [--runtime <directory>] [--no-pack]
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/system/ExecutablePath.hpp"
#include "rtype/engine/exporting/GameExport.hpp"
#include "rtype/engine/exporting/LibrarySelection.hpp"

namespace {
constexpr int kErrorCode = 84;  ///< What the program returns when it cannot export.

// The version the build gives (xmake/modules/engine_version.lua), as for aylo-runtime.
// NOLINTBEGIN(cppcoreguidelines-macro-usage): the build passes it as a macro; this is the default without it.
#ifndef RTYPE_ENGINE_VERSION
#define RTYPE_ENGINE_VERSION "0.0.0"
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

#ifdef NDEBUG
constexpr bool kIsDebug = false;
#else
constexpr bool kIsDebug = true;
#endif

constexpr std::string_view kUsage =
    "usage: aylo-export <game> <output> [--name <name>] [--runtime <directory>] [--no-pack]\n"
    "  <game>       the game folder, with its project.luau\n"
    "  <output>     where the exported game goes, as <output>/<name>/\n"
    "  --name       the game's name, its folder and its program (default: the game folder's)\n"
    "  --runtime    the runtime to ship (default: the zip's runtime/, next to tools/)\n"
    "  --no-pack    ship the game as its folder, game/, instead of one game.pak\n";

/// @brief The command line, read.
struct Options {
    std::filesystem::path game;                    ///< The game folder.
    std::filesystem::path output;                  ///< Where the export goes.
    std::string name;                              ///< The game's name; empty: the default.
    std::optional<std::filesystem::path> runtime;  ///< The runtime; empty: the default.
    bool pack{true};                               ///< The game in one pack.
};

/// @return The options of @p args, or nothing if they are not valid (the usage is printed).
std::optional<Options> parse(const std::vector<std::string>& args) {
    Options options;
    std::vector<std::string> positional;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args.at(i);
        if (arg == "--help" || arg == "-h") {
            std::cout << kUsage;
            return std::nullopt;
        }
        if (arg == "--no-pack") {
            options.pack = false;
            continue;
        }
        if (arg == "--name" || arg == "--runtime") {
            if (i + 1 >= args.size()) {
                std::cerr << std::format("{} needs a value\n{}", arg, kUsage);
                return std::nullopt;
            }
            if (arg == "--name") {
                options.name = args.at(++i);
            } else {
                options.runtime = std::filesystem::path{args.at(++i)};
            }
            continue;
        }
        if (arg.starts_with("--")) {
            std::cerr << std::format("unknown option {}\n{}", arg, kUsage);
            return std::nullopt;
        }
        positional.push_back(arg);
    }
    if (positional.size() != 2) {
        std::cerr << kUsage;
        return std::nullopt;
    }
    options.game = positional.at(0);
    options.output = positional.at(1);
    return options;
}

/// @return The runtime the export ships by default: the zip's `runtime/` when the tool is in its `tools/`, else the
/// folder of the tool (a build folder).
std::filesystem::path defaultRuntime() {
    std::filesystem::path here = rtype::core::system::getExecutableDirectory();
    std::filesystem::path zip = here.parent_path() / "runtime";
    if (std::filesystem::is_regular_file(zip / rtype::engine::exporting::runtimeProgramName())) {
        return zip;
    }
    return here;
}

/// @return @p bytes in megabytes, with one decimal.
std::string megabytes(const std::uintmax_t bytes) {
    constexpr double kMegabyte = 1024.0 * 1024.0;
    return std::format("{:.1f} MB", static_cast<double>(bytes) / kMegabyte);
}
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span rawArgs{argv, static_cast<std::size_t>(argc)};
        std::vector<std::string> args;
        if (!rawArgs.empty()) {  // Some platforms start a program with no argv[0].
            args.assign(rawArgs.begin() + 1, rawArgs.end());
        }
        const std::optional<Options> options = parse(args);
        if (!options) {
            return args.empty() || (args.front() != "--help" && args.front() != "-h") ? kErrorCode : 0;
        }
        const rtype::engine::exporting::ExportReport report = rtype::engine::exporting::exportGame({
            .game = options->game,
            .runtime = options->runtime.value_or(defaultRuntime()),
            .output = options->output,
            .name = options->name,
            .engineVersion = RTYPE_ENGINE_VERSION,
            .debug = kIsDebug,
            .pack = options->pack,
        });
        std::cout << std::format("Exported to {} ({})\n", report.folder.generic_string(), megabytes(report.bytes));
        for (const rtype::engine::exporting::SelectedLibrary& library : report.libraries) {
            std::string reasons;
            for (const std::string& reason : library.reasons) {
                reasons += std::format("{}{}", reasons.empty() ? "" : "; ", reason);
            }
            std::cout << std::format("  {}: {}\n", library.file.filename().generic_string(), reasons);
        }
        for (const std::string& warning : report.warnings) {
            std::cerr << "warning: " << warning << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
