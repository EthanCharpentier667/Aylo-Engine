/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameName
*/

#include "core/layout/GameName.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>

#include "core/layout/Layout.hpp"

namespace aylo::core::layout {

std::string gameName(const std::filesystem::path& game) {
    const std::filesystem::path folder = game.filename().empty() ? game.parent_path() : game;
    if (folder.filename() == kGameFolder && !folder.parent_path().filename().empty()) {
        return folder.parent_path().filename().string();
    }
    return folder.filename().string();
}

bool isFileName(const std::string_view name) noexcept {
    constexpr std::string_view kForbidden = "/\\:*?\"<>|";
    constexpr unsigned char kFirstPrintable = 0x20;
    return !name.empty() && name != "." && name != ".." && name.find_first_of(kForbidden) == std::string_view::npos &&
           std::ranges::none_of(name, [](const char c) { return static_cast<unsigned char>(c) < kFirstPrintable; });
}

}  // namespace aylo::core::layout
