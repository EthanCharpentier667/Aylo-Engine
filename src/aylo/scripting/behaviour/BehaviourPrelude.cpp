/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehaviourPrelude
*/

#include "BehaviourPrelude.hpp"

#include <array>
#include <string_view>

namespace aylo::scripting::behaviour {
namespace {
constexpr auto kPrelude = std::to_array<char>({
#include "BehaviourPrelude.luau.h"
});
}  // namespace

std::string_view getBehaviourPrelude() noexcept { return std::string_view{kPrelude.data(), kPrelude.size() - 1}; }
}  // namespace aylo::scripting::behaviour
