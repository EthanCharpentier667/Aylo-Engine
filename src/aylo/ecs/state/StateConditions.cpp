/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateConditions
*/

#include "aylo/ecs/state/StateConditions.hpp"

#include <string>
#include <utility>

#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/state/States.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
System::RunCondition inState(std::string name, std::string value) {
    return [name = std::move(name), value = std::move(value)](const World& world) {
        const auto* states = world.getResources().get<States>();
        return states != nullptr && states->isIn(name, value);
    };
}

System::RunCondition onEnter(std::string name, std::string value) {
    return [name = std::move(name), value = std::move(value)](const World& world) {
        const auto* states = world.getResources().get<States>();
        return states != nullptr && states->isEntering(name, value);
    };
}

System::RunCondition onExit(std::string name, std::string value) {
    return [name = std::move(name), value = std::move(value)](const World& world) {
        const auto* states = world.getResources().get<States>();
        return states != nullptr && states->isLeaving(name, value);
    };
}
}  // namespace aylo::ecs
