/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DirectorySource
*/

#include "core/vfs/DirectorySource.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"

namespace rtype::core::vfs {
DirectorySource::DirectorySource(std::filesystem::path directory) : _directory{std::move(directory)} {}

std::string DirectorySource::read(const std::string_view path) const {
    return asset::readTextFile(asset::resolveAssetPath(_directory, std::filesystem::path{path}));
}

bool DirectorySource::exists(const std::string_view path) const noexcept {
    try {
        std::error_code error;
        return std::filesystem::is_regular_file(asset::resolveAssetPath(_directory, std::filesystem::path{path}),
                                                error);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> DirectorySource::list(const std::string_view folder) const {
    std::filesystem::path root;
    try {
        root = folder.empty() ? _directory : asset::resolveAssetPath(_directory, std::filesystem::path{folder});
    } catch (const asset::exceptions::AssetLoadException&) {
        return {};
    }
    const std::filesystem::path base = _directory.empty() ? std::filesystem::path{"."} : _directory;
    std::vector<std::string> files;
    std::error_code error;
    for (auto entry = std::filesystem::recursive_directory_iterator{root.empty() ? base : root, error};
         !error && entry != std::filesystem::recursive_directory_iterator{}; entry.increment(error)) {
        if (entry->is_regular_file(error)) {
            files.push_back(entry->path().lexically_relative(base).generic_string());
        }
    }
    std::ranges::sort(files);
    return files;
}

std::optional<std::filesystem::path> DirectorySource::directory() const { return _directory; }
}  // namespace rtype::core::vfs
