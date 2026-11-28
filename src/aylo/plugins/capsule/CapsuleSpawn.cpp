/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleSpawn
*/

#include "CapsuleSpawn.hpp"

#include <format>
#include <span>

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/SpawnedCapsule.hpp"
#include "plugins/capsule/detail/CapsuleSpawner.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace aylo::plugins::capsule {
namespace {
/// @return The capsule with this id, checked to spawn.
/// @throws exceptions::CapsuleException When no capsule has it, or it cannot be spawned (checkCapsule()).
const Capsule& requireSpawnable(const Capsules& capsules, CapsuleId id) {
    const Capsule* capsule = capsules.find(id);
    if (capsule == nullptr) {
        throw exceptions::CapsuleException{std::format("No capsule has the id {:#x}", id.value)};
    }
    detail::checkCapsule(capsules, *capsule);  // before anything is recorded
    return *capsule;
}
}  // namespace

ecs::Commands::EntityCommands spawnCapsule(ecs::Commands& commands, const Capsules& capsules, CapsuleId id) {
    const Capsule& capsule = requireSpawnable(capsules, id);
    const ecs::Entity root = detail::spawnTree(capsules, capsule, commands, ecs::Entity{}, false);
    // A root that reaches the clients tells them its capsule, for its client part. Only the side that sends it does.
    if (capsules.keeps(ecs::Presence::kServerOnly) && detail::rootReplicates(capsules, capsule)) {
        commands.add(root, SpawnedCapsule{.capsule = capsule.getId()});
    }
    return commands.entity(root);
}

void spawnClientPart(ecs::Commands& commands, const Capsules& capsules, ecs::Entity root, CapsuleId id) {
    const Capsule& capsule = requireSpawnable(capsules, id);
    static_cast<void>(detail::spawnTree(capsules, capsule, commands, root, false, true));
}

ecs::Commands::EntityCommands loadCapsule(ecs::Commands& commands, const Capsules& capsules, LoadedCapsule& loaded,
                                          CapsuleId id) {
    const Capsule& capsule = requireSpawnable(capsules, id);  // before the old capsule goes
    if (loaded.root != ecs::Entity{}) {
        commands.despawn(loaded.root);  // does nothing at the flush if the game despawned it already
    }
    const ecs::Entity root = detail::spawnTree(capsules, capsule, commands, ecs::Entity{}, false);
    loaded = LoadedCapsule{.root = root, .capsule = capsule.getId()};
    return commands.entity(root);
}

void reloadCapsule(ecs::Commands& commands, const Capsules& capsules, ecs::Entity root, const CapsuleInstance& instance,
                   std::span<const ecs::Entity> children) {
    const Capsule& capsule = requireSpawnable(capsules, instance.capsule);
    for (const ecs::Entity child : children) {
        commands.despawn(child);
    }
    static_cast<void>(detail::spawnTree(capsules, capsule, commands, root, instance.nested));
}
}  // namespace aylo::plugins::capsule
