/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BuiltinShaders
*/

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "plugins/shader/Export.hpp"

namespace aylo::plugins::shader {

/// @brief What starts the name of a shader the engine gives games, instead of a file of the game:
/// `"@aylo/bloom.glsl"`.
inline constexpr std::string_view kBuiltinShaderPrefix = "@aylo/";

/// @return The source of the engine's shader @p name ("@aylo/bloom.glsl", "@aylo/tonemap.glsl"), written in the same
/// dialect as a game's (docs/Shaders.md), or nothing if @p name names none.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::optional<std::string_view> findBuiltinShader(std::string_view name) noexcept;

/// @return The names of the engine's shaders, "@aylo/bloom.glsl, @aylo/tonemap.glsl, ...", for messages.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::string listBuiltinShaders();

/// @brief The engine's libraries a shader includes, for messages.
inline constexpr std::string_view kBuiltinLibraryNames = "@aylo/noise.glsl, @aylo/color.glsl, @aylo/math.glsl";

/// @return The source of the engine's library @p name ("@aylo/noise.glsl", "@aylo/color.glsl", "@aylo/math.glsl"),
/// functions a shader takes with `#include` (docs/Shaders.md), or nothing if @p name names none.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::optional<std::string_view> findBuiltinLibrary(std::string_view name) noexcept;

}  // namespace aylo::plugins::shader
