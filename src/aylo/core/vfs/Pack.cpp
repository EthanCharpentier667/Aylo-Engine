/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Pack
*/

#include "core/vfs/Pack.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <istream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/FileSource.hpp"
#include "core/vfs/exceptions/PackException.hpp"

namespace aylo::core::vfs {
namespace {
/// @brief What every pack starts with.
constexpr std::array<char, 8> kMagic{'A', 'Y', 'L', 'O', 'P', 'A', 'K', '\0'};
/// @brief The bytes before the table: the magic, the version and the number of entries.
constexpr std::uint64_t kHeaderSize = kMagic.size() + 4 + 4;
/// @brief The bytes of an entry of the table besides its path: the path's length, the offset and the size.
constexpr std::uint64_t kEntryFixedSize = 4 + 8 + 8;
/// @brief The longest path a pack holds, so a damaged length is refused before anything is read.
constexpr std::uint32_t kMaxPathSize = 4096;
/// @brief The bytes copied at once when a file is written into a pack.
constexpr std::size_t kCopyBufferSize = std::size_t{1} << 16U;

/// @return @p path as a pack names it (normalized, forward slashes), or nothing if it is not a relative path that stays
/// in the pack.
std::optional<std::string> packPath(const std::string_view path) {
    if (path.empty()) {
        return std::nullopt;
    }
    const std::filesystem::path given{path};
    if (given.has_root_name() || given.has_root_directory() || path.starts_with('/') || path.starts_with('\\')) {
        return std::nullopt;
    }
    std::string name = asset::normalizePath(given);
    if (name.empty() || name == "." || name == ".." || name.starts_with("../") || name.starts_with('/')) {
        return std::nullopt;
    }
    while (name.ends_with('/')) {
        name.pop_back();
    }
    return name;
}

/// @brief Appends @p value to @p out, little endian, on @p size bytes.
void putInteger(std::string& out, const std::uint64_t value, const std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        out.push_back(static_cast<char>((value >> (8U * i)) & 0xFFU));
    }
}

/// @brief Reads a little-endian integer of @p size bytes at the stream's position.
/// @return It, or nothing if the stream ends first.
std::optional<std::uint64_t> getInteger(std::istream& in, const std::size_t size) {
    std::array<unsigned char, 8> bytes{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): a stream reads chars.
    if (!in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size))) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < size; ++i) {
        value |= static_cast<std::uint64_t>(bytes.at(i)) << (8U * i);
    }
    return value;
}

/// @return The size of an entry's data.
/// @throws exceptions::PackException If it is a file that cannot be read.
std::uint64_t sizeOf(const PackEntry& entry) {
    if (const auto* bytes = std::get_if<std::string>(&entry.data)) {
        return bytes->size();
    }
    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(std::get<std::filesystem::path>(entry.data), error);
    if (error) {
        throw exceptions::PackException{std::format("Cannot read '{}' for the pack: {}",
                                                    std::get<std::filesystem::path>(entry.data).generic_string(),
                                                    error.message())};
    }
    return size;
}

/// @brief Writes the data of @p entry, @p size bytes, to @p out.
/// @throws exceptions::PackException If its file cannot be read, or no longer has @p size bytes.
void writeData(std::ofstream& out, const PackEntry& entry, const std::uint64_t size) {
    if (const auto* bytes = std::get_if<std::string>(&entry.data)) {
        out.write(bytes->data(), static_cast<std::streamsize>(bytes->size()));
        return;
    }
    const auto& file = std::get<std::filesystem::path>(entry.data);
    std::ifstream in{file, std::ios::binary};
    std::array<char, kCopyBufferSize> buffer{};
    std::uint64_t copied = 0;
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::uint64_t>(in.gcount());
        out.write(buffer.data(), static_cast<std::streamsize>(count));
        copied += count;
    }
    if (copied != size) {
        throw exceptions::PackException{
            std::format("'{}' changed while it was packed ({} bytes, then {})", file.generic_string(), size, copied)};
    }
}
/// @return Why @p file is refused as a pack: @p why.
exceptions::PackException notAPack(const std::filesystem::path& file, const std::string_view why) {
    return exceptions::PackException{std::format("'{}' is not a game pack: {}", file.generic_string(), why)};
}

/// @brief One entry of a pack's table, read and checked.
struct TableEntry {
    std::string name;      ///< Its path, normalized.
    std::uint64_t offset;  ///< From the start of the pack.
    std::uint64_t size;    ///< In bytes.
};

/// @return The entry @p index of the table of the pack @p file, at the stream's position, checked against the @p size
/// bytes of the pack.
/// @throws exceptions::PackException If it is cut short, its path is too long, leaves the pack or is not normalized, or
/// it goes past the end of the pack.
TableEntry readTableEntry(std::istream& in, const std::filesystem::path& file, const std::uint64_t index,
                          const std::uint64_t size) {
    const std::optional<std::uint64_t> length = getInteger(in, 4);
    if (!length || *length == 0 || *length > kMaxPathSize) {
        throw notAPack(file, std::format("the path of its entry {} has an invalid length", index));
    }
    std::string name(static_cast<std::size_t>(*length), '\0');
    if (!in.read(name.data(), static_cast<std::streamsize>(name.size()))) {
        throw notAPack(file, "its table is longer than the file");
    }
    const std::optional<std::uint64_t> offset = getInteger(in, 8);
    const std::optional<std::uint64_t> bytes = getInteger(in, 8);
    if (!offset || !bytes) {
        throw notAPack(file, "its table is longer than the file");
    }
    const std::optional<std::string> path = packPath(name);
    if (!path || *path != name) {
        throw notAPack(file, std::format("its entry '{}' is not a path that stays in it", name));
    }
    if (*offset > size || *bytes > size - *offset) {
        throw notAPack(file, std::format("its entry '{}' goes past the end of the file", name));
    }
    return TableEntry{.name = std::move(name), .offset = *offset, .size = *bytes};
}
}  // namespace

void writePack(const std::filesystem::path& file, const std::vector<PackEntry>& entries) {
    std::vector<std::string> paths;
    std::vector<std::uint64_t> sizes;
    std::set<std::string, std::less<>> seen;
    std::uint64_t tableSize = 0;
    for (const PackEntry& entry : entries) {
        const std::optional<std::string> path = packPath(entry.path);
        if (!path || path->size() > kMaxPathSize) {
            throw exceptions::PackException{
                std::format("Invalid path '{}' for the pack: it must be relative and stay in it", entry.path)};
        }
        if (!seen.insert(*path).second) {
            throw exceptions::PackException{std::format("The pack holds '{}' twice", *path)};
        }
        tableSize += kEntryFixedSize + path->size();
        sizes.push_back(sizeOf(entry));
        paths.push_back(*path);
    }
    std::string header{kMagic.data(), kMagic.size()};
    putInteger(header, kPackVersion, 4);
    putInteger(header, entries.size(), 4);
    std::uint64_t offset = kHeaderSize + tableSize;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        putInteger(header, paths.at(i).size(), 4);
        header += paths.at(i);
        putInteger(header, offset, 8);
        putInteger(header, sizes.at(i), 8);
        offset += sizes.at(i);
    }
    std::ofstream out{file, std::ios::binary | std::ios::trunc};
    if (!out) {
        throw exceptions::PackException{std::format("Cannot write the pack '{}'", file.generic_string())};
    }
    out.write(header.data(), static_cast<std::streamsize>(header.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        writeData(out, entries.at(i), sizes.at(i));
    }
    out.close();
    if (!out) {
        throw exceptions::PackException{std::format("Cannot write the pack '{}'", file.generic_string())};
    }
}

PackSource::PackSource(std::filesystem::path file, const std::uint64_t start) : _file{std::move(file)} {
    std::error_code error;
    const std::uintmax_t fileSize = std::filesystem::file_size(_file, error);
    if (error) {
        throw exceptions::PackException{
            std::format("Cannot open the pack '{}': {}", _file.generic_string(), error.message())};
    }
    if (start > fileSize || fileSize - start < kHeaderSize) {
        throw notAPack(_file, "it is too short");
    }
    const std::uint64_t size = fileSize - start;
    std::ifstream in{_file, std::ios::binary};
    in.seekg(static_cast<std::streamoff>(start));
    std::array<char, kMagic.size()> magic{};
    if (!in.read(magic.data(), magic.size()) || magic != kMagic) {
        throw notAPack(_file, "it does not start like one");
    }
    const std::optional<std::uint64_t> version = getInteger(in, 4);
    if (!version || *version != kPackVersion) {
        throw notAPack(_file, std::format("its version is {}, the engine reads {}", version.value_or(0), kPackVersion));
    }
    const std::optional<std::uint64_t> count = getInteger(in, 4);
    if (!count || *count > (size - kHeaderSize) / (kEntryFixedSize + 1)) {
        throw notAPack(_file, "its table is longer than the file");
    }
    for (std::uint64_t i = 0; i < *count; ++i) {
        TableEntry entry = readTableEntry(in, _file, i, size);
        if (!_entries.emplace(entry.name, Entry{.offset = start + entry.offset, .size = entry.size}).second) {
            throw notAPack(_file, std::format("it holds '{}' twice", entry.name));
        }
    }
}

std::string PackSource::read(const std::string_view path) const {
    const std::optional<std::string> name = packPath(path);
    if (!name) {
        throw asset::exceptions::AssetLoadException{std::format("'{}' is outside the assets directory", path)};
    }
    const auto found = _entries.find(*name);
    if (found == _entries.end()) {
        throw asset::exceptions::AssetLoadException{
            std::format("cannot open '{}': the pack '{}' has no such file", *name, _file.generic_string())};
    }
    std::ifstream in{_file, std::ios::binary};
    std::string bytes(static_cast<std::size_t>(found->second.size), '\0');
    in.seekg(static_cast<std::streamoff>(found->second.offset));
    if (!in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) {
        throw asset::exceptions::AssetLoadException{
            std::format("cannot read '{}' from the pack '{}'", *name, _file.generic_string())};
    }
    return bytes;
}

bool PackSource::exists(const std::string_view path) const noexcept {
    try {
        const std::optional<std::string> name = packPath(path);
        return name && _entries.contains(*name);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> PackSource::list(const std::string_view folder) const {
    std::string prefix;
    if (!folder.empty()) {
        const std::optional<std::string> name = packPath(folder);
        if (!name) {
            return {};
        }
        prefix = *name + "/";
    }
    std::vector<std::string> files;
    for (auto entry = _entries.lower_bound(prefix); entry != _entries.end() && entry->first.starts_with(prefix);
         ++entry) {
        files.push_back(entry->first);
    }
    return files;
}

std::optional<std::filesystem::path> PackSource::directory() const { return std::nullopt; }

const std::filesystem::path& PackSource::getFile() const noexcept { return _file; }

SubSource::SubSource(std::shared_ptr<const FileSource> parent, std::string folder)
    : _parent{std::move(parent)}, _folder{std::move(folder)} {}

std::optional<std::string> SubSource::inParent(const std::string_view path) const {
    const std::optional<std::string> name = packPath(path);
    if (!name) {
        return std::nullopt;
    }
    return _folder.empty() ? *name : std::format("{}/{}", _folder, *name);
}

std::string SubSource::read(const std::string_view path) const {
    const std::optional<std::string> name = inParent(path);
    if (!name) {
        throw asset::exceptions::AssetLoadException{std::format("'{}' is outside the assets directory", path)};
    }
    return _parent->read(*name);
}

bool SubSource::exists(const std::string_view path) const noexcept {
    try {
        const std::optional<std::string> name = inParent(path);
        return name && _parent->exists(*name);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> SubSource::list(const std::string_view folder) const {
    std::string inside = _folder;
    if (!folder.empty()) {
        const std::optional<std::string> name = inParent(folder);
        if (!name) {
            return {};
        }
        inside = *name;
    }
    const std::string prefix = _folder.empty() ? std::string{} : _folder + "/";
    std::vector<std::string> files;
    for (const std::string& file : _parent->list(inside)) {
        if (file.starts_with(prefix)) {
            files.push_back(file.substr(prefix.size()));
        }
    }
    return files;
}

std::optional<std::filesystem::path> SubSource::directory() const {
    const std::optional<std::filesystem::path> parent = _parent->directory();
    if (!parent) {
        return std::nullopt;
    }
    return _folder.empty() ? *parent : *parent / _folder;
}
}  // namespace aylo::core::vfs
