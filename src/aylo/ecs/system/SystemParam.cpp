/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SystemParam
*/

#include "SystemParam.hpp"

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
SystemParam<Commands&>::State SystemParam<Commands&>::init(World& /*world*/) noexcept { return {}; }

Commands& SystemParam<Commands&>::get(State& /*state*/, const SystemContext& context) noexcept {
    return *context.commands;
}

void SystemParam<Commands&>::addAccess(const State& /*state*/, SystemAccess& /*access*/) noexcept {}
}  // namespace aylo::ecs
