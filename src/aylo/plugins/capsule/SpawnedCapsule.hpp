/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SpawnedCapsule
*/

#pragma once

#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::capsule {
/// @brief The capsule a root the server spawned comes from, sent to the clients with it, so that each one spawns the
/// part of the capsule the server cannot send: the root's values of components that are not replicated (its sprite, its
/// sound, its collider) and its `side=client` nodes.
/// @details spawnCapsule() gives it to the root on the server (and standalone), when the root has a replicated
/// component: a capsule that never reaches the clients stays off the network. On a client, the system
/// `aylo.Capsule.SpawnReplicated` spawns that part into the roots that have one and no CapsuleInstance yet, then gives
/// them their CapsuleInstance, so a capsule the client spawned itself, or one both sides load, is never spawned twice.
struct SpawnedCapsule {
    core::asset::AssetId capsule;  ///< The capsule (Capsule::getId()).
};
AYLO_ECS_COMPONENT(SpawnedCapsule, "aylo.SpawnedCapsule", capsule);
}  // namespace aylo::plugins::capsule

/// @brief SpawnedCapsule is sent once, with the root, whoever registers it first.
template <>
inline constexpr aylo::ecs::ComponentOptions
    aylo::ecs::kDefaultComponentOptions<aylo::plugins::capsule::SpawnedCapsule> = aylo::ecs::ComponentOptions{
        .presence = aylo::ecs::Presence::kBoth, .replication = aylo::ecs::ReplicationMode::kSpawnOnly};
