/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateConditions
*/

#pragma once

#include <string>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/system/System.hpp"

namespace aylo::ecs {
/// @return A run condition true while the machine @p name of the world's States is in @p value: the system is skipped
/// in every other value, and before the machine entered its first one.
/// @code
/// app.addSystem(aylo::ecs::Phase::kFixedUpdate, "game.spawnWaves", &spawnWaves)
///     .runIf(aylo::ecs::inState("game.Phase", "Playing"));
/// @endcode
[[nodiscard]] AYLO_ECS_API System::RunCondition inState(std::string name, std::string value);

/// @return A run condition true once, when the machine @p name enters @p value: for a system of the StateTransition
/// phase, which then runs in the enter pass.
[[nodiscard]] AYLO_ECS_API System::RunCondition onEnter(std::string name, std::string value);

/// @return A run condition true once, when the machine @p name leaves @p value: for a system of the StateTransition
/// phase, which then runs in the exit pass, before the systems that enter the new value.
[[nodiscard]] AYLO_ECS_API System::RunCondition onExit(std::string name, std::string value);
}  // namespace aylo::ecs
