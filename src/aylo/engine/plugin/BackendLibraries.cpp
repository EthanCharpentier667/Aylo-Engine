/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BackendLibraries
*/

#include "aylo/engine/plugin/BackendLibraries.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "aylo/core/config/Settings.hpp"
#include "aylo/engine/Exceptions.hpp"
#include "aylo/engine/plugin/SharedLibrary.hpp"

namespace aylo::engine::plugin {
namespace {
/// @return The backend a library file provides: what follows the last `-` of its name (`glfw` for
/// `libaylo-platform-glfw.dylib`).
std::string backendOf(const std::filesystem::path& file) {
    const std::string stem = file.stem().string();
    const std::size_t dash = stem.rfind('-');
    return dash == std::string::npos ? stem : stem.substr(dash + 1);
}
}  // namespace

std::vector<std::filesystem::path> backendLibraries(const std::filesystem::path& directory,
                                                    const core::config::Settings& settings,
                                                    const std::span<const std::string_view> keys) {
    std::vector<std::filesystem::path> available;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{directory, error}) {
        if (entry.path().extension() == SharedLibrary::getExtension()) {
            available.push_back(entry.path());
        }
    }
    std::ranges::sort(available);
    std::vector<std::filesystem::path> chosen;
    for (const std::string_view key : keys) {
        if (!settings.has(key)) {
            continue;
        }
        const std::string name = settings.getString(key, "");
        const auto found = std::ranges::find(available, name, &backendOf);
        if (found == available.end()) {
            std::string names;
            for (const std::filesystem::path& file : available) {
                names += std::format("{}'{}'", names.empty() ? "" : ", ", backendOf(file));
            }
            throw exceptions::RuntimeError{
                std::format("Invalid setting '{}': no backend library provides '{}' (found in '{}': {})", key, name,
                            directory.generic_string(), names.empty() ? "none" : names)};
        }
        if (std::ranges::find(chosen, *found) == chosen.end()) {
            chosen.push_back(*found);
        }
    }
    std::ranges::sort(chosen);
    return chosen;
}
}  // namespace aylo::engine::plugin
