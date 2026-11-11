/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ParticleEmitter
*/

#pragma once

#include <cstdint>
#include <type_traits>

#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep

/// @brief A ParticleSimulation field is reflected as its underlying std::uint8_t.
template <>
struct aylo::ecs::FieldTypeOf<aylo::contract::renderer::ParticleSimulation>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};

namespace aylo::plugins::particle {
/// @brief Plays an effect at its entity's GlobalTransform, as the component "rtype.ParticleEmitter". Client-side.
struct ParticleEmitter {
    core::asset::AssetId effect;     ///< From ParticleEffectAssets::load() or add().
    bool playing{true};              ///< false stops emitting; living particles finish their life.
    std::int32_t layer{0};           ///< Draw order among the sprites.
    float rateScale{1.0F};           ///< Multiplies the effect's rate: more smoke as a ship gets damaged.
    bool overrideSimulation{false};  ///< true: simulation replaces the effect's choice.
    contract::renderer::ParticleSimulation simulation{
        contract::renderer::ParticleSimulation::kAuto};  ///< If overridden.
    std::uint32_t seed{0};                               ///< 0: picked when it starts.
    core::asset::AssetId material;  ///< Replaces the effect's material if set. Its `instance uniform` values come
                                    ///< from the entity's MaterialInstance, shared by the emitter's particles.
};

AYLO_ECS_COMPONENT(ParticleEmitter, "rtype.ParticleEmitter", effect, playing, layer, rateScale, overrideSimulation,
                   simulation, seed, material);

/// @brief Despawns its entity once its ParticleEmitter has finished (ParticlesFinished), as the component
/// "rtype.ParticleOneShot": for fire-and-forget effects, such as the ones SpawnParticles plays. Its effect must not
/// loop, or the entity stays. An entity whose effect cannot be read is despawned too.
struct ParticleOneShot {};
AYLO_ECS_COMPONENT(ParticleOneShot, "rtype.ParticleOneShot");

}  // namespace aylo::plugins::particle
