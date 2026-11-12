/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SaveStore
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "plugins/persistence/Export.hpp"

namespace aylo::plugins::persistence {

/// @brief The outcome of a SaveStore operation: no exception, a failure is a value.
struct SaveResult {
    bool ok{false};     ///< The operation succeeded.
    std::string data;   ///< read(): the saved text; empty otherwise.
    std::string error;  ///< Why it failed: `invalid_name`, `not_found`, `corrupt` or `io: <detail>`; empty on success.
};

/// @brief Saves named opaque texts as files of one directory. A write is atomic (a temporary file, then a rename), so
/// a crash leaves the previous save whole. Each file starts with a short header (format, size, checksum) so that a
/// truncated or damaged file is reported as `corrupt`. What the text means (schema version, migration) is the game's.
class AYLO_PLUGIN_PERSISTENCE_API SaveStore {
  public:
    /// @param directory Where the saves live; created on the first write.
    explicit SaveStore(std::filesystem::path directory);

    /// @return Whether @p name can name a save: 1 to 64 letters, digits, `_`, `-` or `.`, not starting with a dot.
    [[nodiscard]] static bool isValidName(std::string_view name);

    /// @return The text saved under @p name, or an error value.
    [[nodiscard]] SaveResult read(std::string_view name) const;
    /// @brief Replaces the save @p name with @p data atomically; on failure the old save is untouched.
    [[nodiscard]] SaveResult write(std::string_view name, std::string_view data) const;
    /// @brief Deletes the save @p name; deleting a save that does not exist succeeds.
    [[nodiscard]] SaveResult remove(std::string_view name) const;
    /// @return Whether a file exists for @p name (it may still be corrupt).
    [[nodiscard]] bool exists(std::string_view name) const;

    /// @brief Copies the saves of @p other here, if this directory does not exist yet and @p other does: how the saves
    /// of a folder the game used before are kept. @p other is left as it was; a save that cannot be copied is skipped.
    /// @return The number of saves copied.
    [[nodiscard]] std::size_t adopt(const std::filesystem::path& other) const noexcept;

    /// @return The directory of the saves.
    [[nodiscard]] const std::filesystem::path& getDirectory() const noexcept;

  private:
    [[nodiscard]] std::filesystem::path pathOf(std::string_view name) const;

    std::filesystem::path _directory;
};

AYLO_ECS_RESOURCE(SaveStore, "aylo.SaveStore");

}  // namespace aylo::plugins::persistence
