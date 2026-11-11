/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Parent
*/

#pragma once

#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {
/// @brief This entity's parent. Set it with `commands.entity(child).add(Parent{.entity = parent})` or
/// World::setParent(), remove it with `commands.entity(child).remove<Parent>()` or World::removeParent(), never with
/// Components::add() or remove(): the World keeps the parent's Children in sync.
struct Parent {
    Entity entity;  ///< The parent.
};
AYLO_ECS_COMPONENT(Parent, "rtype.Parent", entity);

/// @brief The options Parent is always registered with, whoever does it first: Transforms are relative, so a client
/// needs the hierarchy to place what the server sends.
inline constexpr ComponentOptions kParentOptions{.presence = Presence::kBoth,
                                                 .replication = ReplicationMode::kEveryChange};
}  // namespace aylo::ecs
