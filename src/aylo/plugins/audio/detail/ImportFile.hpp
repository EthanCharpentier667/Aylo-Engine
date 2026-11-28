/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ImportFile
*/

#pragma once

#include <string>
#include <string_view>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::plugins::audio::detail {
/// @brief The extension added to a sound's path to name its import file: `maint.ogg.import`.
inline constexpr std::string_view kImportFileExtension = ".import";

/// @brief Applies the import file of the sound at @p path, if it has one, to @p desc: `{ "stream": true }` streams it.
/// @details An option the file sets to true is true, whatever @p desc says: what the file asks for holds wherever the
/// sound is loaded from (a setup form, a capsule, a path a script names).
/// @param source Where the sound and its import file are.
/// @param path The sound's path, normalized.
/// @return @p desc with the file's options.
/// @throws contract::audiodevice::exceptions::AudioLoadException When the file is not a JSON object, has a key it does
/// not know or a value of the wrong type; the message names the file.
[[nodiscard]] contract::audiodevice::SoundDesc applyImportFile(const core::vfs::FileSource& source,
                                                               const std::string& path,
                                                               contract::audiodevice::SoundDesc desc);
}  // namespace aylo::plugins::audio::detail
