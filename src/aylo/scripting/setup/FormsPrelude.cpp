/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FormsPrelude
*/

#include "FormsPrelude.hpp"

#include <array>
#include <string_view>

namespace aylo::scripting::setup {
namespace {
constexpr auto kPrelude = std::to_array<char>({
#include "FormsPrelude.luau.h"
});
}  // namespace

std::string_view getFormsPrelude() noexcept { return std::string_view{kPrelude.data(), kPrelude.size() - 1}; }
}  // namespace aylo::scripting::setup
