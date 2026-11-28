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
/// @param clientPart Whether `root` is a root the server spawned, on a client: it gets only its values of components
/// that are not replicated, and only its `side=client` children are spawned (with their subtrees, as usual); the
/// others are the server's, and come through replication.
/// @return The root.
/// @pre checkCapsule() passed.
AYLO_PLUGIN_CAPSULE_API ecs::Entity spawnTree(const Capsules& capsules, const Capsule& capsule, ecs::Commands& commands,
                                              ecs::Entity root, bool nested, bool clientPart = false);

/// @return Whether a spawn of `capsule` on the registry's side gives its root a replicated component (its own, or the
/// root of the capsule it instances): such a root reaches the clients, which then spawn its client part.
[[nodiscard]] AYLO_PLUGIN_CAPSULE_API bool rootReplicates(const Capsules& capsules, const Capsule& capsule);
}  // namespace aylo::plugins::capsule::detail
