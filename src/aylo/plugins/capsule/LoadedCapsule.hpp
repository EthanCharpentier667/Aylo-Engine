/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LoadedCapsule
*/

#pragma once

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"

namespace aylo::plugins::capsule {
/// @brief The capsule loadCapsule() loaded last, the "aylo.LoadedCapsule" resource: the level being played.
/// @details Loading another capsule despawns this one's root (and its descendants) first. Entities outside its tree,
/// the player or a manager, are not touched. Apart from the Capsules, so a load writes only this and spawns keep
/// reading the Capsules alongside.
struct LoadedCapsule {
    ecs::Entity root;              ///< The loaded capsule's root; the null entity before the first load.
    core::asset::AssetId capsule;  ///< The loaded capsule.
};
AYLO_ECS_RESOURCE(LoadedCapsule, "aylo.LoadedCapsule");
}  // namespace aylo::plugins::capsule
