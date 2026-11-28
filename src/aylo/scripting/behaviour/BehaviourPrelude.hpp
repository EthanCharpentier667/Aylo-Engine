/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehaviourPrelude
*/

#pragma once

#include <string_view>

namespace aylo::scripting::behaviour {
/// @brief Name of the prelude in error messages.
inline constexpr std::string_view kBehaviourPreludeName = "engine.behaviour";

/// @return The Luau source of `engine.behaviour`: a script attached to entities, compiled to a component and one
/// system per hook (`<name>.onUpdate`...) with `engine.ecs.component` and `engine.ecs.system`, so it adds no concept
/// to the ECS. It runs after the forms prelude, whose `engine.forms` it reads its declaration with.
[[nodiscard]] std::string_view getBehaviourPrelude() noexcept;
}  // namespace aylo::scripting::behaviour
