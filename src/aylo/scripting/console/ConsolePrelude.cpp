/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ConsolePrelude
*/

#include "ConsolePrelude.hpp"

#include <array>
#include <string_view>

namespace aylo::scripting::console {
namespace {
constexpr auto kPrelude = std::to_array<char>({
#include "ConsolePrelude.luau.h"
});
}  // namespace

std::string_view getConsolePrelude() noexcept { return std::string_view{kPrelude.data(), kPrelude.size() - 1}; }
}  // namespace aylo::scripting::console
