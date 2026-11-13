/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BackendLibraries
*/

#pragma once

#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "aylo/core/config/Settings.hpp"
#include "aylo/engine/Export.hpp"

namespace aylo::engine::plugin {
/// @return The backend libraries of @p directory the settings name, sorted: for each key of @p keys that @p settings
/// has (`platform`, `renderer`, `audio.backend`), the library whose file name ends with `-<name>`
/// (`aylo-platform-glfw` for `"glfw"`, `aylo-backend-raylib` for `"raylib"`). A process opens no other backend: two
/// that carry the same third-party library (raylib's own GLFW and platform-glfw) never meet in one process, where their
/// copies would clash (macOS registers Objective-C classes by name, once per process).
/// @throws exceptions::RuntimeError If no library of @p directory provides a named backend: the message names the
/// setting and the backends found.
[[nodiscard]] AYLO_ENGINE_API std::vector<std::filesystem::path> backendLibraries(
    const std::filesystem::path& directory, const core::config::Settings& settings,
    std::span<const std::string_view> keys);
}  // namespace aylo::engine::plugin
