/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleInstance
*/

#pragma once

#include <cstdint>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::capsule {
/// @brief The root of a spawned capsule: which capsule, at which version, and whether another capsule spawned it.
/// @details Every spawned capsule's root has one, nested ones included. Despawning the root despawns the capsule (its
/// descendants go with it). A reload respawns the outermost capsule a nested root belongs to, so the outer node's
/// values stay on it.
struct CapsuleInstance {
    core::asset::AssetId capsule;  ///< The capsule (Capsule::getId()).
    std::uint32_t version{0};      ///< The capsule's version when it was spawned (Capsule::getVersion()).
    bool nested{false};            ///< Spawned as a node of another capsule, not by itself.
};
AYLO_ECS_COMPONENT(CapsuleInstance, "rtype.CapsuleInstance", capsule, version, nested);
}  // namespace aylo::plugins::capsule
