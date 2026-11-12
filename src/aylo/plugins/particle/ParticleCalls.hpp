/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ParticleCalls
*/

#pragma once

#include <cstdint>
#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <map>
#include <numbers>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "plugins/particle/ParticleEffect.hpp"

namespace aylo::plugins::particle {

inline constexpr std::string_view kBeginEffectCall = "aylo.particle.beginEffect";  ///< Starts a ParticleEffectDraft.
inline constexpr std::string_view kAddBurstCall = "aylo.particle.addBurst";        ///< Adds a burst to a draft.
inline constexpr std::string_view kAddColorKeyCall = "aylo.particle.addColorKey";  ///< Adds to its colorOverLife.
inline constexpr std::string_view kAddCurveKeyCall = "aylo.particle.addCurveKey";  ///< Adds to its size or speed.
inline constexpr std::string_view kAddEffectCall = "aylo.particle.addEffect";      ///< ParticleEffectAssets::add().

/// @brief The effects a script is describing, by name: `aylo.particle.beginEffect` starts one, the `add*Key` and
/// `addBurst` calls fill it, and `aylo.particle.addEffect` checks it and moves it to the ParticleEffectAssets. An
/// effect is built whole before it is stored, so a mistake in its description never leaves half of it playing.
/// The ECS resource "aylo.ParticleEffectDrafts", inserted by the ParticlePlugin.
struct ParticleEffectDrafts {
    std::map<std::string, ParticleEffect, std::less<>> effects;  ///< Begun and not added yet.

    ParticleEffectDrafts() = default;
    ~ParticleEffectDrafts() = default;
    ParticleEffectDrafts(const ParticleEffectDrafts& other) = delete;
    ParticleEffectDrafts& operator=(const ParticleEffectDrafts& other) = delete;
    // noexcept, as a resource must be: MSVC's std::map does not promise it on its own.
    ParticleEffectDrafts(ParticleEffectDrafts&& other) noexcept = default;
    ParticleEffectDrafts& operator=(ParticleEffectDrafts&& other) noexcept = default;
};
AYLO_ECS_RESOURCE(ParticleEffectDrafts, "aylo.ParticleEffectDrafts");

/// @brief The arguments of `aylo.particle.beginEffect`: everything of an effect but its lists, which the calls below
/// add. Each default is the one of ParticleEffect. Beginning a name again starts it over.
struct BeginEffectArguments {
    ecs::String name;                ///< What the effect is known by: its id is AssetId::fromPath(name).
    ecs::String simulation{"auto"};  ///< "auto", "gpu" or "cpu".
    std::uint32_t capacity{1024};    ///< Most particles alive at once.
    ecs::String space{"world"};      ///< "world": left behind; "local": follow the emitter.
    bool sortByAge{false};           ///< Draws the oldest first; always on with the "alpha" blend.
    float rate{0.0F};                ///< Particles per second while playing.
    float duration{1.0F};            ///< Seconds of one cycle.
    bool loop{true};                 ///< true: a new cycle starts after duration.
    ecs::String shape{"point"};      ///< Where particles spawn: "point", "circle" or "rect".
    float radius{0.0F};              ///< "circle": its radius, in world units.
    bool edgeOnly{false};            ///< "circle": on the circle only, as a ring.
    glm::vec2 rectSize{0.0F, 0.0F};  ///< "rect": its width and height, in world units.
    float lifetimeMin{1.0F};         ///< Seconds; above 0.
    float lifetimeMax{1.0F};         ///< Seconds; lifetimeMin or more.
    float speedMin{0.0F};            ///< World units per second, along the angle.
    float speedMax{0.0F};            ///< speedMin or more.
    float angleMin{0.0F};            ///< Direction, in radians.
    float angleMax{2.0F * std::numbers::pi_v<float>};  ///< Every way by default.
    float rotationMin{0.0F};                           ///< Of the quad, in radians, clockwise on screen.
    float rotationMax{0.0F};                           ///< rotationMin or more.
    float angularVelocityMin{0.0F};                    ///< Of the quad, in radians per second.
    float angularVelocityMax{0.0F};                    ///< angularVelocityMin or more.
    float sizeMin{1.0F};                               ///< Width and height of the quad, in world units.
    float sizeMax{1.0F};                               ///< sizeMin or more.
    glm::vec4 color{1.0F, 1.0F, 1.0F, 1.0F};           ///< Multiplied by colorOverLife.
    glm::vec2 gravity{0.0F, 0.0F};                     ///< World units per second squared; +Y is down.
    float drag{0.0F};                                  ///< Slows particles: velocity times exp(-drag * seconds).
    core::asset::AssetId texture;                      ///< From `engine.assets.texture`.
    glm::vec2 sourcePosition{0.0F, 0.0F};              ///< Top left of the part of the texture shown, in pixels.
    glm::vec2 sourceSize{0.0F, 0.0F};                  ///< Its size; 0: the whole texture.
    ecs::String blend{"additive"};                     ///< "additive", "alpha" or "cutout".
    ecs::String alignment{"fixed"};                    ///< "fixed", or "velocity": turned along the velocity.
    float stretch{0.0F};                               ///< "velocity": the quad grows by speed * stretch.
    std::uint32_t flipbookColumns{1};                  ///< Frames per row of the source.
    std::uint32_t flipbookRows{1};                     ///< Rows of frames.
    std::uint32_t flipbookFrames{0};                   ///< Frames played; 0: every cell.
    float flipbookCycles{1.0F};                        ///< Times the frames play over a life.
    core::asset::AssetId material;                     ///< From `engine.assets.material`; the default one if not given.
};
AYLO_ECS_CALL_DATA(BeginEffectArguments, "aylo.particle.BeginEffectArguments", name, simulation, capacity, space,
                   sortByAge, rate, duration, loop, shape, radius, edgeOnly, rectSize, lifetimeMin, lifetimeMax,
                   speedMin, speedMax, angleMin, angleMax, rotationMin, rotationMax, angularVelocityMin,
                   angularVelocityMax, sizeMin, sizeMax, color, gravity, drag, texture, sourcePosition, sourceSize,
                   blend, alignment, stretch, flipbookColumns, flipbookRows, flipbookFrames, flipbookCycles, material);

/// @brief The arguments of `aylo.particle.addBurst`: particles emitted at once, in each cycle.
struct AddBurstArguments {
    ecs::String name;         ///< The effect begun with `aylo.particle.beginEffect`.
    float time{0.0F};         ///< Seconds after the start of the cycle.
    std::uint32_t count{0};   ///< Particles emitted each time.
    std::uint32_t cycles{1};  ///< How many times it happens.
    float interval{0.0F};     ///< Seconds between two times.
};
AYLO_ECS_CALL_DATA(AddBurstArguments, "aylo.particle.AddBurstArguments", name, time, count, cycles, interval);

/// @brief The arguments of `aylo.particle.addColorKey`: a key of the effect's colorOverLife, after the others.
struct AddColorKeyArguments {
    ecs::String name;                         ///< The effect begun with `aylo.particle.beginEffect`.
    float t{0.0F};                            ///< 0 at birth, 1 at death.
    glm::vec4 color{1.0F, 1.0F, 1.0F, 1.0F};  ///< The colour at t, alpha included.
};
AYLO_ECS_CALL_DATA(AddColorKeyArguments, "aylo.particle.AddColorKeyArguments", name, t, color);

/// @brief The arguments of `aylo.particle.addCurveKey`: a key of the effect's sizeOverLife or speedOverLife.
struct AddCurveKeyArguments {
    ecs::String name;           ///< The effect begun with `aylo.particle.beginEffect`.
    ecs::String curve{"size"};  ///< "size" or "speed".
    float t{0.0F};              ///< 0 at birth, 1 at death.
    float value{1.0F};          ///< The multiplier at t.
};
AYLO_ECS_CALL_DATA(AddCurveKeyArguments, "aylo.particle.AddCurveKeyArguments", name, curve, t, value);

/// @brief The arguments of `aylo.particle.addEffect`.
struct AddEffectArguments {
    ecs::String name;  ///< The effect begun with `aylo.particle.beginEffect`.
};
AYLO_ECS_CALL_DATA(AddEffectArguments, "aylo.particle.AddEffectArguments", name);

/// @brief The result of `aylo.particle.addEffect`.
struct EffectResult {
    core::asset::AssetId effect;  ///< For a ParticleEmitter or a SpawnParticles.
};
AYLO_ECS_CALL_DATA(EffectResult, "aylo.particle.EffectResult", effect);

/// @brief Registers the particle engine calls on @p app, for scripts: `aylo.particle.beginEffect`, `addBurst`,
/// `addColorKey`, `addCurveKey` and `addEffect`. They throw a contract::renderer::exceptions::ParticleException
/// naming what is wrong (an unknown name or draft, an invalid effect). Called by the ParticlePlugin.
void addParticleCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin (`engine.assets.particle`) on @p registry: its library does it on
/// every side, so a server gets the ids of what it does not build, and so does building the plugin.
void addParticleForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::particle
