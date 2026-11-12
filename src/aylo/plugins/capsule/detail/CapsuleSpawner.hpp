/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleSpawner
*/

#pragma once

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"

namespace aylo::plugins::capsule {
class Capsules;
}  // namespace aylo::plugins::capsule

namespace aylo::plugins::capsule::detail {
/// @brief Checks that a spawn of `capsule` on the registry's side can go through: every instance it spawns exists and
/// none contains itself. A spawn checks first, so it records everything or nothing.
/// @throws exceptions::CapsuleException Naming the capsule, the node and the missing or repeated capsule.
AYLO_PLUGIN_CAPSULE_API void checkCapsule(const Capsules& capsules, const Capsule& capsule);

/// @brief Records the spawn of `capsule`'s nodes in order: a node of the other side is skipped with its subtree; a node
/// is a new entity, or the root its instance spawns; its kept values are added (over an instanced root's); it is
/// parented to its parent's entity. The root gets a CapsuleInstance last.
/// @param root An existing entity to spawn the root into (a reload), or the null entity for a new one.
/// @param nested Whether another capsule spawns this one as a node.
/// @return The root.
/// @pre checkCapsule() passed.
AYLO_PLUGIN_CAPSULE_API ecs::Entity spawnTree(const Capsules& capsules, const Capsule& capsule, ecs::Commands& commands,
                                              ecs::Entity root, bool nested);
}  // namespace aylo::plugins::capsule::detail
