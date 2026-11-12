/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameName
*/

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace aylo::core::layout {

/// @return The name of the game in the folder @p game: `MyGame` for `MyGame/game` (kGameFolder), else the folder's own
/// name. What a game is called when it says nothing: the name of its export, the folder of its saves.
[[nodiscard]] std::string gameName(const std::filesystem::path& game);

/// @return Whether @p name can name a file or a folder on every system: not empty, not `.` or `..`, and none of
/// `/\:*?"<>|` or a control character.
[[nodiscard]] bool isFileName(std::string_view name) noexcept;

}  // namespace aylo::core::layout
