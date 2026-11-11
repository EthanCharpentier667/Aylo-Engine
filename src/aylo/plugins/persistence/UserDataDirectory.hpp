/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** UserDataDirectory
*/

#pragma once

#include <filesystem>

#include "plugins/persistence/Export.hpp"

namespace aylo::plugins::persistence {

/// @return The current user's data directory, where a game keeps what must survive (saves): `%APPDATA%` on Windows,
/// `~/Library/Application Support` on macOS, `$XDG_DATA_HOME` or `~/.local/share` elsewhere. Empty when the
/// environment does not tell.
[[nodiscard]] AYLO_PLUGIN_PERSISTENCE_API std::filesystem::path getUserDataDirectory();

}  // namespace aylo::plugins::persistence
