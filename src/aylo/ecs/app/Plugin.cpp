/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Plugin
*/

#include "Plugin.hpp"

#include <span>
#include <string_view>

namespace aylo::ecs {
Plugin::~Plugin() = default;  // defined here: the class's vtable and type information live in the library

std::span<const std::string_view> Plugin::getDependencies() const noexcept { return {}; }

void Plugin::finish(App& /*app*/) {}
}  // namespace aylo::ecs
