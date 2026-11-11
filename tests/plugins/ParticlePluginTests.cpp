/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ParticlePluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <span>
#include <utility>
#include <variant>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/particle/CpuSimulation.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/particle/ParticleEvents.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/particle/ParticleSystems.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/transform/Transform.hpp"

using aylo::contract::renderer::ParticleDraw;
using aylo::contract::renderer::ParticleInstance;
using aylo::contract::renderer::ParticleSimulation;
using aylo::contract::renderer::ParticleSystemId;
using aylo::core::asset::AssetId;
using aylo::plugins::particle::ParticleEffect;
using aylo::plugins::particle::ParticleEffectAssets;
using aylo::plugins::particle::ParticleEmitter;
using aylo::plugins::particle::ParticleOneShot;
using aylo::plugins::particle::ParticlesFinished;
using aylo::plugins::particle::ParticleSystems;
using aylo::plugins::particle::SpawnParticles;
using aylo::plugins::transform::Transform;

namespace {
/// @brief An App with the GraphicsPlugin over the fake backend, 0.1 second per frame, the texture spark.png loaded and
/// the effects "burst" (one burst of 8 particles living 0.25 seconds, once) and "stream" (10 particles per second,
/// looping).
class ParticleApp {
  public:
    explicit ParticleApp(bool gpuParticles = false)
        : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        _state->gpuParticles = gpuParticles;
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
        _app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        _texture = world().getResources().getMut<aylo::plugins::graphics::TextureAssets>()->load(
            "spark.png", aylo::test::plugins::twoPixelPng());

        ParticleEffect burst;
        burst.texture = _texture;
        burst.desc.loop = false;
        burst.desc.duration = 0.05F;
        burst.desc.lifetime = {.min = 0.25F, .max = 0.25F};
        burst.desc.bursts.push_back({.count = 8});
        _burst = effects().add("burst", std::move(burst));

        ParticleEffect stream;
        stream.texture = _texture;
        stream.desc.rate = 10.0F;
        _stream = effects().add("stream", std::move(stream));
    }
    ~ParticleApp() = default;
    ParticleApp(const ParticleApp& other) = delete;
    ParticleApp& operator=(const ParticleApp& other) = delete;
    ParticleApp(ParticleApp&& other) = delete;
    ParticleApp& operator=(ParticleApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    [[nodiscard]] ParticleEffectAssets& effects() { return *world().getResources().getMut<ParticleEffectAssets>(); }
    [[nodiscard]] ParticleSystems& systems() { return *world().getResources().getMut<ParticleSystems>(); }
    [[nodiscard]] AssetId burst() const noexcept { return _burst; }
    [[nodiscard]] AssetId stream() const noexcept { return _stream; }

    /// @brief Spawns an entity at @p position playing @p emitter.
    aylo::ecs::Entity spawnEmitter(ParticleEmitter emitter, glm::vec2 position = {0.0F, 0.0F}) {
        const aylo::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, Transform{.position = position});
        world().getComponents().add(entity, emitter);
        return entity;
    }

    /// @return The particle draws of one more frame.
    std::vector<ParticleDraw> drawFrame() {
        _state->particleDraws.clear();
        _state->drawOrder.clear();
        _app.runFrame();
        return _state->particleDraws;
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
    AssetId _texture;                                               ///< spark.png.
    AssetId _burst;                                                 ///< The effect "burst".
    AssetId _stream;                                                ///< The effect "stream".
};

/// @return The number of CPU instances @p draw hands over (0 for a GPU system).
std::size_t countInstances(const ParticleDraw& draw) {
    const auto* instances = std::get_if<std::span<const ParticleInstance>>(&draw.particles);
    return instances != nullptr ? instances->size() : 0;
}
}  // namespace

/// @return The material "hit", of a shader with the instance value `flash`, 0.25 by default.
AssetId addMaterial(ParticleApp& app) {
    app.state().shaders = true;
    aylo::ecs::Resources& resources = app.world().getResources();
    const AssetId shader = resources.getMut<aylo::plugins::shader::ShaderAssets>()->add(
        "flash", "instance uniform float flash = 0.25;\nvoid fragment() { COLOR.rgb += flash; }\n");
    return resources.getMut<aylo::plugins::shader::MaterialAssets>()->add("hit", {.shader = shader});
}

TEST(ParticlePlugin, DrawsAnEmitterWithItsMaterialAndInstanceValues) {
    ParticleApp app;
    const AssetId material = addMaterial(app);
    const aylo::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream(), .material = material});
    aylo::plugins::shader::MaterialInstance instance;
    instance.values[0].x = 0.75F;
    app.world().getComponents().add(entity, instance);
    static_cast<void>(app.drawFrame());
    const std::vector<ParticleDraw> draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_NE(draws.at(0).material, aylo::contract::renderer::MaterialId{});
    EXPECT_FLOAT_EQ(draws.at(0).instanceValues[0].x, 0.75F);
}

TEST(ParticlePlugin, DrawsAnEmitterWithItsEffectsMaterialAndTheShadersDefaults) {
    ParticleApp app;
    ParticleEffect effect;
    effect.texture = app.effects().get(app.stream())->texture;
    effect.desc.rate = 10.0F;
    effect.material = addMaterial(app);
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.effects().add("hit", std::move(effect))}));
    static_cast<void>(app.drawFrame());
    const std::vector<ParticleDraw> draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_NE(draws.at(0).material, aylo::contract::renderer::MaterialId{});
    EXPECT_FLOAT_EQ(draws.at(0).instanceValues[0].x, 0.25F);
}

TEST(ParticlePlugin, DrawsAnEmitterWithTheDefaultMaterialUntilItsIsCreated) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream(), .material = AssetId::fromPath("hit")}));
    static_cast<void>(app.drawFrame());
    const std::vector<ParticleDraw> draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(draws.at(0).material, aylo::contract::renderer::MaterialId{});
}

TEST(ParticlePlugin, ComesWithTheGraphicsPlugin) {
    ParticleApp app;
    EXPECT_TRUE(app.app().hasPlugin(aylo::plugins::particle::ParticlePlugin::kName));
    EXPECT_TRUE(app.world().getResources().has<ParticleEffectAssets>());
    EXPECT_TRUE(app.world().getResources().has<ParticleSystems>());
    EXPECT_TRUE(app.world().getComponents().find("aylo.ParticleEmitter"));
    EXPECT_TRUE(app.world().getComponents().find("aylo.ParticleOneShot"));
}

TEST(ParticlePlugin, DrawsACpuEmitterAtItsLayer) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream(), .layer = 3}));
    static_cast<void>(app.drawFrame());  // GlobalTransform is added at the end of this frame
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 1U);
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'p'}));
    EXPECT_TRUE(app.state().createdParticleSystems.empty());
}

TEST(ParticlePlugin, DrawsWithTheLookOfTheEffect) {
    ParticleApp app;
    ParticleEffect sparks = *app.effects().get(app.stream());
    sparks.blend = aylo::contract::renderer::BlendMode::kAlpha;
    sparks.alignment = aylo::contract::renderer::ParticleAlignment::kVelocity;
    sparks.stretch = 0.1F;
    sparks.flipbook = {.columns = 4, .rows = 2, .cycles = 2.0F};
    const AssetId id = app.effects().add("sparks", std::move(sparks));
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = id}));
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(draws.at(0).blend, aylo::contract::renderer::BlendMode::kAlpha);
    EXPECT_EQ(draws.at(0).alignment, aylo::contract::renderer::ParticleAlignment::kVelocity);
    EXPECT_FLOAT_EQ(draws.at(0).stretch, 0.1F);
    EXPECT_EQ(draws.at(0).flipbook,
              (aylo::contract::renderer::ParticleFlipbook{.columns = 4, .rows = 2, .cycles = 2.0F}));
}

TEST(ParticlePlugin, EmitsTheRateOverTheFrames) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream()}));
    static_cast<void>(app.drawFrame());
    for (int i = 0; i < 4; ++i) {
        static_cast<void>(app.drawFrame());
    }
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 5U);
}

TEST(ParticlePlugin, AStoppedEmitterEmitsNothing) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream(), .playing = false}));
    static_cast<void>(app.drawFrame());
    EXPECT_TRUE(app.drawFrame().empty());
    EXPECT_EQ(app.systems().getCount(), 1U);
}

TEST(ParticlePlugin, AnUnknownEffectPlaysNothing) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = AssetId::fromPath("missing")}));
    static_cast<void>(app.drawFrame());
    EXPECT_TRUE(app.drawFrame().empty());
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, RemovingTheEmitterStopsIt) {
    ParticleApp app;
    const aylo::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream()});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(app.systems().getCount(), 1U);
    app.world().getComponents().remove<ParticleEmitter>(entity);
    EXPECT_TRUE(app.drawFrame().empty());
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, ChangingTheEffectRestartsTheEmitter) {
    ParticleApp app;
    const aylo::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream()});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    app.world().getComponents().getMut<ParticleEmitter>(entity)->effect = app.burst();
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 8U);
    EXPECT_EQ(app.systems().find(entity)->effect, app.burst());
}

TEST(ParticlePlugin, ReplacingTheEffectRestartsTheEmittersPlayingIt) {
    ParticleApp app;
    const aylo::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.burst()});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());  // the burst is out: nothing new to emit
    ParticleEffect bigger = *app.effects().get(app.burst());
    bigger.desc.bursts.at(0).count = 12;
    app.effects().replace(app.burst(), std::move(bigger));
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 12U);  // started again, with the new burst
    EXPECT_EQ(app.systems().find(entity)->effectVersion, 1U);
}

TEST(ParticlePlugin, KeepsPlayingWhileTheSimulationIsPaused) {
    ParticleApp app;
    app.app().getSchedules().setPaused(true);  // an editor editing: no gameplay, but what is drawn still moves
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream()}));
    static_cast<void>(app.drawFrame());
    const auto first = app.drawFrame();
    ASSERT_EQ(first.size(), 1U);
    const std::size_t before = countInstances(first.at(0));
    const auto second = app.drawFrame();
    ASSERT_EQ(second.size(), 1U);
    EXPECT_GT(countInstances(second.at(0)), before);
}

TEST(ParticlePlugin, AOneShotEmitterFinishesAndIsDespawned) {
    ParticleApp app;
    const aylo::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.burst()});
    app.world().getComponents().add(entity, ParticleOneShot{});
    aylo::ecs::EventCursor cursor;
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(countInstances(app.drawFrame().at(0)), 8U);
    int sent = 0;
    for (int i = 0; i < 3; ++i) {
        static_cast<void>(app.drawFrame());
        for (const ParticlesFinished& finished : app.world().getEvents().read<ParticlesFinished>(cursor)) {
            sent += finished.entity == entity ? 1 : 0;
        }
    }
    EXPECT_EQ(sent, 1);
    EXPECT_FALSE(app.world().isAlive(entity));
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, SpawnParticlesPlaysAOneShotEffect) {
    ParticleApp app;
    app.world().getEvents().send(SpawnParticles{.effect = app.burst(), .position = {40.0F, 20.0F}, .layer = 1});
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    ASSERT_EQ(countInstances(draws.at(0)), 8U);
    EXPECT_EQ(std::get<std::span<const ParticleInstance>>(draws.at(0).particles)[0].position,
              (glm::vec2{40.0F, 20.0F}));
    for (int i = 0; i < 4; ++i) {
        static_cast<void>(app.drawFrame());
    }
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, RunsOnTheGpuWhenTheRendererCan) {
    ParticleApp app{true};
    const aylo::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream()}, {5.0F, 6.0F});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(app.state().createdParticleSystems.size(), 1U);
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_TRUE(std::holds_alternative<ParticleSystemId>(draws.at(0).particles));
    ASSERT_EQ(app.state().simulations.size(), 2U);
    EXPECT_EQ(app.state().simulations.at(1).position, (glm::vec2{5.0F, 6.0F}));
    EXPECT_EQ(app.state().simulations.at(1).firstParticle, app.state().simulations.at(0).spawnCount);

    app.world().despawn(entity);
    static_cast<void>(app.drawFrame());
    ASSERT_EQ(app.state().destroyedParticleSystems.size(), 1U);
    EXPECT_EQ(app.state().destroyedParticleSystems.at(0), std::get<ParticleSystemId>(draws.at(0).particles));
}

TEST(ParticlePlugin, SortsAlphaEffectsByAge) {
    ParticleApp app{true};
    ParticleEffect smoke = *app.effects().get(app.stream());
    smoke.blend = aylo::contract::renderer::BlendMode::kAlpha;
    const AssetId id = app.effects().add("smoke", std::move(smoke));
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream()}));
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = id}));
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    ASSERT_EQ(app.state().createdParticleSystems.size(), 2U);
    EXPECT_NE(app.state().createdParticleSystems.at(0).sortByAge, app.state().createdParticleSystems.at(1).sortByAge);
}

TEST(ParticlePlugin, FallsBackToTheCpuWhenTheGpuIsAskedButMissing) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(
        ParticleEmitter{.effect = app.stream(), .overrideSimulation = true, .simulation = ParticleSimulation::kGpu}));
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 2U);
}

TEST(ParticlePlugin, TheEmitterCanForceTheCpu) {
    ParticleApp app{true};
    static_cast<void>(app.spawnEmitter(
        ParticleEmitter{.effect = app.stream(), .overrideSimulation = true, .simulation = ParticleSimulation::kCpu}));
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    EXPECT_TRUE(app.state().createdParticleSystems.empty());
    EXPECT_EQ(countInstances(app.drawFrame().at(0)), 2U);
}
