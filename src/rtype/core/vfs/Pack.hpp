/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Pack
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "core/Export.hpp"
#include "core/vfs/FileSource.hpp"

namespace rtype::core::vfs {
/// @brief The extension of a game's pack: `game.pak`.
inline constexpr std::string_view kPackExtension = ".pak";

/// @brief The version of the pack format packs are written in; a reader refuses another.
inline constexpr std::uint32_t kPackVersion = 1;

/// @brief One file to put in a pack: its path in it, and its bytes or the file on disk that holds them.
struct PackEntry {
    std::string path;                                       ///< In the pack: relative, with forward slashes.
    std::variant<std::filesystem::path, std::string> data;  ///< A file on disk, or the bytes themselves.
};

/// @brief Writes the pack @p file from @p entries: one file a game is exported in (docs/release/export.md, EXP-06).
///
/// @details The format, little endian: the magic `AYLOPAK\0`, the version (u32), the number of entries (u32), then per
/// entry its path (u32 length, then the bytes), its offset and its size (u64 each, the offset from the start of the
/// pack), then the data of every entry, in the order given. Nothing is compressed.
/// @throws exceptions::PackException If a path is not a relative path that stays in the pack (absolute, `..`, empty), a
/// path is given twice, a file on disk cannot be read, or @p file cannot be written.
RTYPE_CORE_API void writePack(const std::filesystem::path& file, const std::vector<PackEntry>& entries);

/// @brief The files of a pack, read as a FileSource: what an exported game reads.
///
/// @details The pack's table is read and checked when it is opened, its files only when they are read, each time from
/// the file, so the pack is never held in memory. A pack is data from outside the engine: a damaged or hostile one is
/// refused when it is opened, with the file and the reason, and never read past its end.
class RTYPE_CORE_API PackSource final : public FileSource {
  public:
    /// @brief Opens the pack @p file, which starts at the byte @p start of it (0: the pack is the whole file; more: it
    /// is appended to another, such as a program).
    /// @throws exceptions::PackException If it is missing, not a pack, of another version, or damaged (a table or an
    /// entry past the end of the file, a path that is absolute, leaves the pack or is given twice).
    explicit PackSource(std::filesystem::path file, std::uint64_t start = 0);
    ~PackSource() override = default;
    PackSource(const PackSource& other) = delete;
    PackSource& operator=(const PackSource& other) = delete;
    PackSource(PackSource&& other) = delete;
    PackSource& operator=(PackSource&& other) = delete;

    [[nodiscard]] std::string read(std::string_view path) const override;
    [[nodiscard]] bool exists(std::string_view path) const noexcept override;
    [[nodiscard]] std::vector<std::string> list(std::string_view folder) const override;
    /// @return Nothing: a pack holds no file on disk.
    [[nodiscard]] std::optional<std::filesystem::path> directory() const override;

    /// @return The pack's file.
    [[nodiscard]] const std::filesystem::path& getFile() const noexcept;

  private:
    /// @brief Where an entry's bytes are in the file.
    struct Entry {
        std::uint64_t offset;  ///< From the start of the file.
        std::uint64_t size;    ///< In bytes.
    };

    std::filesystem::path _file;                         ///< The pack.
    std::map<std::string, Entry, std::less<>> _entries;  ///< Its files, by path.
};

/// @brief The files of a folder of another source, as a source of their own: `assets/` of a pack, with `ship.png` for
/// `assets/ship.png`.
class RTYPE_CORE_API SubSource final : public FileSource {
  public:
    /// @param parent The source the files are in; never null.
    /// @param folder The folder, relative to @p parent, without a trailing slash (`assets`).
    SubSource(std::shared_ptr<const FileSource> parent, std::string folder);
    ~SubSource() override = default;
    SubSource(const SubSource& other) = delete;
    SubSource& operator=(const SubSource& other) = delete;
    SubSource(SubSource&& other) = delete;
    SubSource& operator=(SubSource&& other) = delete;

    /// @throws core::asset::exceptions::AssetLoadException As FileSource::read(); a path that leaves the folder too.
    [[nodiscard]] std::string read(std::string_view path) const override;
    [[nodiscard]] bool exists(std::string_view path) const noexcept override;
    [[nodiscard]] std::vector<std::string> list(std::string_view folder) const override;
    /// @return The folder in the parent's directory, if the parent has one.
    [[nodiscard]] std::optional<std::filesystem::path> directory() const override;

  private:
    /// @return @p path in the parent, or nothing if it leaves the folder.
    [[nodiscard]] std::optional<std::string> inParent(std::string_view path) const;

    std::shared_ptr<const FileSource> _parent;  ///< Where the files are.
    std::string _folder;                        ///< The folder in it.
};
}  // namespace rtype::core::vfs
