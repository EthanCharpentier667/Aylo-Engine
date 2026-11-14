/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ParticleEmitter
*/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
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
/// @brief The names scripts and capsule files give the values of a ParticleSimulation field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::contract::renderer::ParticleSimulation> {
    static constexpr std::array<std::string_view, 3> kNames{"auto", "gpu", "cpu"};
};

namespace aylo::plugins::particle {
/// @brief Plays an effect at its entity's GlobalTransform2D, as the component "aylo.ParticleEmitter". Client-side.
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

AYLO_ECS_COMPONENT(ParticleEmitter, "aylo.ParticleEmitter", effect, playing, layer, rateScale, overrideSimulation,
                   simulation, seed, material);

/// @brief Despawns its entity once its ParticleEmitter has finished (ParticlesFinished), as the component
/// "aylo.ParticleOneShot": for fire-and-forget effects, such as the ones SpawnParticles plays. Its effect must not
/// loop, or the entity stays. An entity whose effect cannot be read is despawned too.
struct ParticleOneShot {};
AYLO_ECS_COMPONENT(ParticleOneShot, "aylo.ParticleOneShot");

}  // namespace aylo::plugins::particle
