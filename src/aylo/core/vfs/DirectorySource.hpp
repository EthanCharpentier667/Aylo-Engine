/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DirectorySource
*/

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Export.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::core::vfs {
/// @brief The files of a folder on disk, with the checks of core::asset::resolveAssetPath(): what a game reads in
/// development, and every program without an Engine.
class AYLO_CORE_API DirectorySource final : public FileSource {
  public:
    /// @param directory The folder; empty: the working directory, and nothing is checked (a program without a game).
    explicit DirectorySource(std::filesystem::path directory);
    ~DirectorySource() override = default;
    DirectorySource(const DirectorySource& other) = delete;
    DirectorySource& operator=(const DirectorySource& other) = delete;
    DirectorySource(DirectorySource&& other) = delete;
    DirectorySource& operator=(DirectorySource&& other) = delete;

    [[nodiscard]] std::string read(std::string_view path) const override;
    [[nodiscard]] bool exists(std::string_view path) const noexcept override;
    [[nodiscard]] std::vector<std::string> list(std::string_view folder) const override;
    /// @return The folder given, even when it is empty.
    [[nodiscard]] std::optional<std::filesystem::path> directory() const override;

  private:
    std::filesystem::path _directory;  ///< The folder.
};
}  // namespace aylo::core::vfs
