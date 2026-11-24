/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderIncludes
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/shader/Export.hpp"

namespace aylo::plugins::shader::detail {

/// @brief How deep `#include`s may nest: an included file including another is one level more.
inline constexpr std::size_t kMaxIncludeDepth = 16;

/// @brief Reads a game file a shader includes, by its path in the assets: its text, or nothing if there is none.
using IncludeReader = std::function<std::optional<std::string>(const std::string& path)>;

/// @brief A shader with its `#include`s replaced by the files they name, and where each of its lines comes from.
struct ExpandedShader {
    /// @brief Where a line of `text` was written.
    struct Origin {
        std::uint32_t file{0};  ///< Index in `files`.
        std::uint32_t line{0};  ///< Its line in that file, from 1.
    };

    std::string text;                ///< The shader, each `#include` line followed by the file it names.
    std::vector<std::string> files;  ///< The shader's name, then each file included, once, in their order.
    std::vector<Origin> lines;       ///< By line of `text`, from 0.

    /// @return Line @p line of `text` (from 1) as a reader finds it: "line 4", or "line 2 of 'shaders/lib.glsl'".
    [[nodiscard]] AYLO_PLUGIN_SHADER_API std::string locate(std::uint32_t line) const;

    /// @return @p message with every "line N:" it holds (the parser's and glslang's) located by locate().
    [[nodiscard]] AYLO_PLUGIN_SHADER_API std::string locateLines(std::string_view message) const;
};

/// @return @p source with its `#include "path"` lines expanded, each file once (a second include of a file is
/// skipped, so two libraries may include a third).
/// @details A path starting with `@aylo/` names a library of the engine (findBuiltinLibrary()). Any other is looked for
/// next to the file that includes it, then from the assets' root, through @p read. A file included is expanded too.
/// @param name The shader's name, for messages and to find what it includes next to it.
/// @param read Reads the game's files; empty: the shader can include only the engine's libraries.
/// @throws exceptions::ShaderException Naming the file and the line of the `#include`: a file not found, an include of
/// a file that is including it (a cycle), more than kMaxIncludeDepth levels, or a line it cannot read.
[[nodiscard]] AYLO_PLUGIN_SHADER_API ExpandedShader expandIncludes(std::string_view source, std::string_view name,
                                                                   const IncludeReader& read);

}  // namespace aylo::plugins::shader::detail
