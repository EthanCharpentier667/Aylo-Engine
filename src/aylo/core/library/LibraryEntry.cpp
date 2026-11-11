/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LibraryEntry
*/

#include "core/library/LibraryEntry.hpp"

#include <format>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/library/exceptions/LibraryException.hpp"

namespace aylo::core::library {

void LibraryRegistrar::provideNames(const std::string_view name, NameLister lister) {
    _nameListers.insert_or_assign(std::string{name}, std::move(lister));
}

std::map<std::string, std::vector<std::string>, std::less<>> LibraryRegistrar::listNames() const {
    std::map<std::string, std::vector<std::string>, std::less<>> names;
    for (const auto& [service, lister] : _nameListers) {
        names.emplace(service, lister ? lister() : std::vector<std::string>{});
    }
    return names;
}

void checkLibrary(const LibraryEntry& entry, const std::string_view file) {
    if (entry.abiVersion != kLibraryAbiVersion) {
        throw exceptions::LibraryException{
            std::format("Library '{}' was built for the version {}, the engine has {}: rebuild it", file,
                        entry.abiVersion, kLibraryAbiVersion)};
    }
    const std::string_view toolchain = entry.toolchain != nullptr ? entry.toolchain : "";
    if (toolchain != AYLO_LIBRARY_TOOLCHAIN) {
        throw exceptions::LibraryException{
            std::format("Library '{}' was built with '{}', the engine with '{}': rebuild it", file, toolchain,
                        AYLO_LIBRARY_TOOLCHAIN)};
    }
}

}  // namespace aylo::core::library
