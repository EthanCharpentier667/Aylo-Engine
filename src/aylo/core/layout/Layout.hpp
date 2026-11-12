/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Layout
*/

#pragma once

#include <string_view>

/// @brief The names of the folders and files the engine, its tools and its release agree on: the one place they are
/// written in C++. The build and the release write the same names in Lua, in `xmake/modules/layout.lua`: change both
/// together (the release's smoke test checks the layout end to end).
namespace aylo::core::layout {
#ifdef _WIN32
inline constexpr std::string_view kProgramExtension = ".exe";  ///< What a program's file name ends with.
#else
inline constexpr std::string_view kProgramExtension;  ///< What a program's file name ends with.
#endif

/// @name A game
/// @{
inline constexpr std::string_view kGameFolder = "game";           ///< The game folder a program runs by default.
inline constexpr std::string_view kProjectFile = "project.luau";  ///< The settings, in the game folder.
inline constexpr std::string_view kScriptsFolder = "scripts";     ///< The scripts, in the game folder.
inline constexpr std::string_view kAssetsFolder = "assets";       ///< The assets, in the game folder.
inline constexpr std::string_view kPluginsFolder = "plugins";     ///< The plugin libraries, by a program and a game.
inline constexpr std::string_view kPackExtension = ".pak";        ///< A game in one file.
inline constexpr std::string_view kPackFile = "game.pak";         ///< The pack a program runs next to itself.
/// @}

/// @name The runtime: what a game ships (`runtime/` in the release zip, the build folder before)
/// @{
inline constexpr std::string_view kRuntimeFolder = "runtime";               ///< The runtime, in the release zip.
inline constexpr std::string_view kToolsFolder = "tools";                   ///< The tools, next to it in the zip.
inline constexpr std::string_view kRuntimeProgram = "aylo-runtime";         ///< The program, without kProgramExtension.
inline constexpr std::string_view kLibFolder = "lib";                       ///< The libraries that are not plugins.
inline constexpr std::string_view kBackendsFolder = "backends";             ///< The backends, in kPluginsFolder.
inline constexpr std::string_view kVulkanFolder = "vulkan";                 ///< The Vulkan driver's manifest (macOS).
inline constexpr std::string_view kLicensesFolder = "licenses";             ///< The third-party notices.
inline constexpr std::string_view kNoticesFile = "THIRD_PARTY_NOTICES.md";  ///< All of them, in kLicensesFolder.
inline constexpr std::string_view kNoticesByLibraryFolder = "by-library";   ///< Those of each binary.
/// @}

/// @name In the user's directories
/// @{
/// The engine's folder in the user's cache directory (the pipeline cache, the editor's layout), and the default folder
/// of a game's saves in the user's data directory.
inline constexpr std::string_view kUserFolder = "aylo";
/// @}

/// @name Next to a plugin library
/// @{
inline constexpr std::string_view kLinksExtension = ".links";    ///< The plugin libraries it links (the build's).
inline constexpr std::string_view kSignatureExtension = ".sig";  ///< Its signature (`aylo-sign`).
/// @}

/// @name An exported game
/// @{
inline constexpr std::string_view kExportManifest = "export.json";  ///< What was kept, and why.
/// @}
}  // namespace aylo::core::layout
