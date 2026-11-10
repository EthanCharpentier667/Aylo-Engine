/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameExport
*/

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "editor/Export.hpp"

namespace rtype::editor::shipping {
/// @brief What the Export panel asks for: where, under which name, and as a pack or a folder.
struct ExportChoice {
    std::string output;  ///< The folder the export goes in, as `<output>/<name>/`.
    std::string name;    ///< The game's name; empty: the game folder's.
    bool pack{true};     ///< The game in one `game.pak`; false: its folder.
};

/// @brief What an export did, as the Export panel shows it.
struct ExportResult {
    bool ok{false};                      ///< Whether the game was exported.
    std::string folder;                  ///< The exported game.
    std::uintmax_t bytes{0};             ///< Its size.
    std::vector<std::string> libraries;  ///< One line per library kept: its file, then why.
    std::vector<std::string> warnings;   ///< What the export could not do.
    std::string error;                   ///< Why it failed, when it did.
};

/// @brief Exports the game the editor runs. The program sets it (the editor knows no engine): aylo-editor calls the
/// engine's export, the same as `aylo-export` (docs/release/export.md). It may throw; runExport() catches it.
using Exporter = std::function<ExportResult(const ExportChoice&)>;

/// @brief What the Export panel remembers between frames and between Apps.
struct ExportPanelState {
    std::array<char, 1024> output{};   ///< The output folder, as typed.
    std::array<char, 256> name{};      ///< The name, as typed.
    bool pack{true};                   ///< As a pack.
    std::optional<ExportResult> last;  ///< The last export, shown until the next.
};

/// @return What @p exporter did with @p choice. Never throws: an exception, or no exporter, is a failed result with the
/// reason.
[[nodiscard]] RTYPE_EDITOR_API ExportResult runExport(const Exporter& exporter, const ExportChoice& choice);
}  // namespace rtype::editor::shipping
