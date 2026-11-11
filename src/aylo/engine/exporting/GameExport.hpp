/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameExport
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "aylo/engine/Export.hpp"
#include "aylo/engine/exporting/LibrarySelection.hpp"
#include "aylo/engine/plugin/PluginSignature.hpp"

namespace aylo::engine::exporting {
/// @brief What exportGame() makes.
struct ExportRequest {
    std::filesystem::path game;     ///< The game folder, with its `project.luau`.
    std::filesystem::path runtime;  ///< The runtime it ships: the folder of `aylo-runtime`, `lib/` and `plugins/`.
    std::filesystem::path output;   ///< The folder the export goes in, as `<output>/<name>/`.
    std::string name;               ///< The game's name: its folder and its program; empty: the game folder's parent's.
    std::string engineVersion;      ///< The engine's version, written in `export.json`.
    bool debug{false};              ///< `engine.isDebug` when `project.luau` is read: the runtime's build.
    bool pack{true};                ///< The game in one pack (`game.pak`); false: as a folder (`game/`).
    /// @brief What the game's plugin libraries must pass before they are opened to read what they bring: by default
    /// the Engine's own check (plugin::gamePluginVerifier(), their signature in a release build).
    std::function<void(const std::filesystem::path&)> verifyGamePlugin = plugin::gamePluginVerifier();
};

/// @brief What exportGame() did.
struct ExportReport {
    std::filesystem::path folder;            ///< The exported game: `<output>/<name>/`.
    std::vector<SelectedLibrary> libraries;  ///< The plugin and backend libraries kept, with their reasons.
    std::vector<std::string> warnings;       ///< What the export could not do, for the developer to read.
    std::uintmax_t bytes{0};                 ///< The size of the export.
};

/// @brief The name of the program of `aylo-runtime` in a runtime folder, with the platform's extension.
[[nodiscard]] AYLO_ENGINE_API std::string runtimeProgramName();

/// @brief Exports a game: the runtime's program, its `lib/`, only the plugin and backend libraries the game needs
/// (selectLibraries()), the game folder and their notices, in one folder a player runs (docs/release/export.md,
/// EXP-03).
///
/// @details It makes, in `<output>/<name>/`:
/// - `<name>` (`.exe` on Windows): the runtime's program, renamed;
/// - `lib/` as it is in the runtime (on Windows, the libraries next to the program);
/// - `plugins/` and `plugins/backends/`: the libraries kept;
/// - `game.pak`: the game's files (core::vfs::writePack()), its scripts precompiled (luau::Runtime::precompile()),
///   without hidden files such as `.luaurc` and without `plugins/`; or, with `pack` false, `game/`: the game folder
///   as it is, without them;
/// - `game/plugins/`: the libraries of the game's own plugins and their signatures (not their sources), which stay
///   files, where the Engine looks for them next to the pack;
/// - `licenses/THIRD_PARTY_NOTICES.md`: the notices of the binaries kept, from the runtime's `licenses/by-library/`
///   (a build folder has none: the report warns);
/// - `play.sh` (`play.bat` on Windows), which runs the game, and `--server` for a dedicated server;
/// - `export.json`: the engine version, the libraries kept and why, the SHA-256 of every file.
/// @param request What to export.
/// @return What it made.
/// @throws exceptions::RuntimeError If the game or the runtime is not one, the output folder exists and is not empty,
/// the name is not a file name, a script does not compile (its file and the compiler's message), or selectLibraries()
/// refuses the game; nothing is written in that case.
/// @throws std::filesystem::filesystem_error If a file cannot be copied or written.
[[nodiscard]] AYLO_ENGINE_API ExportReport exportGame(const ExportRequest& request);
}  // namespace aylo::engine::exporting
