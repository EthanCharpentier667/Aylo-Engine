/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LibrarySelection
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "aylo/engine/Export.hpp"

namespace aylo::engine::exporting {
/// @brief The folder a library of the runtime comes from, which is the folder it goes to in an export.
enum class LibraryKind : std::uint8_t {
    kPlugin,   ///< `plugins/`, next to the program.
    kBackend,  ///< `plugins/backends/`.
    kGame,     ///< `<game>/plugins/`: the game's own.
};

/// @brief A library a game needs, and why.
struct SelectedLibrary {
    std::filesystem::path file;        ///< Its file, absolute.
    LibraryKind kind{};                ///< Where it comes from.
    std::vector<std::string> reasons;  ///< Why it is kept, sorted: "named in plugins (client)", "required by ...".
};

/// @brief What selectLibraries() reads.
struct SelectionRequest {
    std::filesystem::path game;     ///< The game folder, with its `project.luau`.
    std::filesystem::path runtime;  ///< The folder of the program, with `plugins/` and `plugins/backends/`.
    std::vector<std::string> args;  ///< The flags `project.luau` sees as `engine.args`, as on the command line.
    bool debug{false};              ///< `engine.isDebug`.
    /// @brief What a library of `<game>/plugins/` must pass before it is opened; empty: none
    /// (plugin::gamePluginVerifier() checks it as the Engine does).
    std::function<void(const std::filesystem::path&)> verifyGamePlugin;
};

/// @brief Finds the plugin and backend libraries a game needs, for an export that ships only those
/// (docs/release/export.md, EXP-01).
///
/// @details `project.luau` is read for the server side, then for the client side, as the Engine reads it. For each side
/// the libraries are loaded and registered one at a time, as the Engine does, to learn which library brings which
/// plugin and which backend. A library is kept when it brings:
/// - a plugin the side names under `plugins`, the transform plugin every game has, or a plugin one of those requires
///   (Plugin::getDependencies(), followed until nothing is added; a plugin is created to read it, never built);
/// - on the client, a backend a backend setting names (Engine::kBackendSettings);
/// - anything, when it is one of the game's own libraries.
///
/// Then a library kept needs the plugin libraries it links, which its `<file>.links` names (written by the build), and
/// those they link. `lib/` is not looked at: an export copies it whole.
/// @param request The game and the runtime.
/// @return The libraries, sorted by kind, then by file.
/// @throws exceptions::RuntimeError If the game has no `project.luau`, a side names a plugin or a backend no library
/// brings, a plugin requires one no library brings, or a library links one that is not in `plugins/`, with the names.
/// @throws core::library::exceptions::LibraryException If a library was built for another engine.
/// @throws luau::exceptions::RuntimeError If `project.luau` fails.
/// @throws core::config::exceptions::SettingsException If a setting a plugin reads when it is created is invalid.
[[nodiscard]] AYLO_ENGINE_API std::vector<SelectedLibrary> selectLibraries(const SelectionRequest& request);
}  // namespace aylo::engine::exporting
