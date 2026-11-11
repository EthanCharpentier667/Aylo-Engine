/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AnimationEvents
*/

#pragma once

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::sprite {

/// @brief An animation that repeats a finite number of times played its last frame: sent once, by the SpritePlugin, in
/// the PostUpdate that saw it.
/// @code
/// void onExploded(aylo::ecs::EventReader<aylo::plugins::sprite::AnimationFinished> finished,
///                 aylo::ecs::Commands& commands) {
///     for (const aylo::plugins::sprite::AnimationFinished& event : finished) { commands.despawn(event.entity); }
/// }
/// @endcode
struct AnimationFinished {
    ecs::Entity entity;              ///< The entity whose SpriteAnimator played it.
    core::asset::AssetId animation;  ///< animationId() of its name.
};
AYLO_ECS_EVENT(AnimationFinished, "rtype.AnimationFinished", entity, animation);

}  // namespace aylo::plugins::sprite
