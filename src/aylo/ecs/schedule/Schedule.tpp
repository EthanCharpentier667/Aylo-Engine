/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Schedule
*/

#include <string>
#include <utility>

namespace aylo::ecs {
template <typename Function, typename... Configs>
Schedule::SystemConfig Schedule::addSystem(std::string name, Function function, Configs... configs) {
    return addSystem(System{*_world, std::move(name), std::move(function), std::move(configs)...});
}
}  // namespace aylo::ecs
