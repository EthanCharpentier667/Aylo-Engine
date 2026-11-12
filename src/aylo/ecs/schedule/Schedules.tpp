/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Schedules
*/

#include <string>
#include <utility>

namespace aylo::ecs {
template <typename Function, typename... Configs>
Schedule::SystemConfig Schedules::addSystem(Phase phase, std::string name, Function function, Configs... configs) {
    return addSystem(phase, System{*_world, std::move(name), std::move(function), std::move(configs)...});
}
}  // namespace aylo::ecs
