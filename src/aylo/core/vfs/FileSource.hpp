/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FileSource
*/

#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Export.hpp"

namespace aylo::core::vfs {
/// @brief Where a game's files are read from: a folder in development, the pack of an exported game
/// (docs/release/export.md, EXP-04). Every reader of the engine (the asset stores, the capsule files) reads through
/// one, so it never knows which.
///
/// @details A path is relative to the source and written with forward slashes (`levels/space.tmj`); it is normalized
/// first (core::asset::normalizePath()), so `a/../b.png` is `b.png`. A path that is absolute or leaves the source is
/// refused, as a missing file is. The source is shared by the readers and outlives them: the Engine keeps it in the
/// world's `rtype.AssetDirectory` resource.
class AYLO_HEADER_ONLY_API FileSource {
  public:
    FileSource() = default;
    virtual ~FileSource() = default;
    FileSource(const FileSource& other) = delete;
    FileSource& operator=(const FileSource& other) = delete;
    FileSource(FileSource&& other) = delete;
    FileSource& operator=(FileSource&& other) = delete;

    /// @return The bytes of the file @p path.
    /// @throws core::asset::exceptions::AssetLoadException If it is missing, cannot be read, or the path is absolute or
    /// leaves the source, naming the path.
    [[nodiscard]] virtual std::string read(std::string_view path) const = 0;

    /// @return Whether @p path is a file of the source. Never throws: a path that leaves it is not one.
    [[nodiscard]] virtual bool exists(std::string_view path) const noexcept = 0;

    /// @return The files under the folder @p folder ("" for the whole source), at any depth, by their path in the
    /// source, sorted; none if there is no such folder.
    [[nodiscard]] virtual std::vector<std::string> list(std::string_view folder) const = 0;

    /// @return The folder on disk the files are in, for what needs a file on disk (hot reload, which watches it);
    /// nothing when they are not files on disk (a pack).
    [[nodiscard]] virtual std::optional<std::filesystem::path> directory() const = 0;
};

/// @return The bytes of the file @p path of @p source, as bytes rather than text (an image, a sound).
/// @throws core::asset::exceptions::AssetLoadException As FileSource::read().
[[nodiscard]] inline std::vector<std::byte> readBytes(const FileSource& source, const std::string_view path) {
    const std::string text = source.read(path);
    std::vector<std::byte> bytes(text.size());
    std::ranges::transform(text, bytes.begin(), [](const char c) { return static_cast<std::byte>(c); });
    return bytes;
}
}  // namespace aylo::core::vfs
