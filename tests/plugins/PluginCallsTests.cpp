/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginCallsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "FakeAudioDevice.hpp"
#include "FakeBackend.hpp"
#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/exceptions/ParticleExceptions.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioCalls.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/exceptions/AudioCallException.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/graphics/TextureCalls.hpp"
#include "plugins/input/InputCalls.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"
#include "plugins/input/exceptions/InputExceptions.hpp"
#include "plugins/particle/ParticleCalls.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsCalls.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/SpriteCalls.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextCalls.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapCalls.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

using aylo::core::asset::AssetId;

namespace {
/// @brief Makes the engine call @p name of @p app, as a script does while it loads, and returns its result.
/// @throws std::invalid_argument If no call has this name; whatever the call throws.
template <typename Result, typename Arguments>
Result call(aylo::ecs::App& app, const std::string_view name, const Arguments& arguments) {
    aylo::ecs::Calls& calls = app.getWorld().getCalls();
    const aylo::ecs::CallInfo* info = calls.findByName(name);
    if (info == nullptr) {
        throw std::invalid_argument{std::format("No engine call '{}'", name)};
    }
    std::array<std::byte, sizeof(Arguments)> bytes{};
    std::memcpy(bytes.data(), &arguments, sizeof(Arguments));
    if constexpr (std::is_void_v<Result>) {
        calls.invoke(app.getWorld(), info->id, bytes, std::span<std::byte>{});
    } else {
        std::array<std::byte, sizeof(Result)> result{};
        calls.invoke(app.getWorld(), info->id, bytes, result);
        Result value{};
        std::memcpy(&value, result.data(), sizeof(Result));
        return value;
    }
}

/// @brief Sends a key press to the InputPlugin, as the backend plugin does after polling the window.
void press(aylo::ecs::App& app, const aylo::contract::platform::input::Key key) {
    app.getWorld().getEvents().send(
        aylo::plugins::input::PlatformEvent{.event = aylo::contract::platform::event::KeyPressed{.key = key}});
}

/// @return The atlas @p id names in @p app.
const aylo::plugins::sprite::SpriteAtlas& atlasOf(aylo::ecs::App& app, const AssetId id) {
    const auto* atlas = app.getWorld().getResources().require<aylo::plugins::sprite::AtlasAssets>().get(id);
    if (atlas == nullptr) {
        throw std::invalid_argument{"No such atlas"};
    }
    return *atlas;
}
}  // namespace

// ---- input ----

TEST(InputCalls, AddActionAndBindDriveAnAction) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    call<void>(app, aylo::plugins::input::kAddActionCall, aylo::plugins::input::AddActionArguments{.name = "shoot"});
    call<void>(app, aylo::plugins::input::kBindCall,
               aylo::plugins::input::BindArguments{.action = "shoot", .control = "Space"});

    press(app, aylo::contract::platform::input::Key::kSpace);
    app.runFrame();

    const auto state = call<aylo::plugins::input::ActionState>(app, aylo::plugins::input::kReadActionCall,
                                                               aylo::plugins::input::ReadActionArguments{"shoot"});
    EXPECT_TRUE(state.held);
    EXPECT_TRUE(state.pressed);
    EXPECT_FALSE(state.released);
    EXPECT_FLOAT_EQ(state.value.x, 1.0F);
}

TEST(InputCalls, BindVectorTakesFourControlsOrTwo) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    call<void>(app, aylo::plugins::input::kAddActionCall,
               aylo::plugins::input::AddActionArguments{.name = "move", .type = "vector2"});
    call<void>(
        app, aylo::plugins::input::kBindVectorCall,
        aylo::plugins::input::BindVectorArguments{.action = "move", .up = "W", .down = "S", .left = "A", .right = "D"});
    call<void>(app, aylo::plugins::input::kBindVectorCall,
               aylo::plugins::input::BindVectorArguments{.action = "move", .x = "GamepadLeftX", .y = "GamepadLeftY"});
    EXPECT_THROW(call<void>(app, aylo::plugins::input::kBindVectorCall,
                            aylo::plugins::input::BindVectorArguments{.action = "move"}),
                 aylo::plugins::input::exceptions::InputException);
    EXPECT_THROW(call<void>(app, aylo::plugins::input::kBindVectorCall,
                            aylo::plugins::input::BindVectorArguments{.action = "move", .up = "W", .x = "MouseDeltaX"}),
                 aylo::plugins::input::exceptions::InputException);

    press(app, aylo::contract::platform::input::Key::kD);
    app.runFrame();

    const auto state = call<aylo::plugins::input::ActionState>(app, aylo::plugins::input::kReadActionCall,
                                                               aylo::plugins::input::ReadActionArguments{"move"});
    EXPECT_FLOAT_EQ(state.value.x, 1.0F);
    EXPECT_FLOAT_EQ(state.value.y, 0.0F);
}

TEST(InputCalls, BindAxisAndScaleBindAControl) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    call<void>(app, aylo::plugins::input::kAddActionCall,
               aylo::plugins::input::AddActionArguments{.name = "steer", .type = "axis"});
    call<void>(app, aylo::plugins::input::kBindAxisCall,
               aylo::plugins::input::BindAxisArguments{.action = "steer", .negative = "Left", .positive = "Right"});
    call<void>(app, aylo::plugins::input::kAddActionCall,
               aylo::plugins::input::AddActionArguments{.name = "boost", .type = "axis"});
    call<void>(app, aylo::plugins::input::kBindCall,
               aylo::plugins::input::BindArguments{.action = "boost", .control = "B", .scale = -2.0F});

    press(app, aylo::contract::platform::input::Key::kLeft);
    press(app, aylo::contract::platform::input::Key::kB);
    app.runFrame();

    const auto steer = call<aylo::plugins::input::ActionState>(app, aylo::plugins::input::kReadActionCall,
                                                               aylo::plugins::input::ReadActionArguments{"steer"});
    EXPECT_FLOAT_EQ(steer.value.x, -1.0F);
    const auto boost = call<aylo::plugins::input::ActionState>(app, aylo::plugins::input::kReadActionCall,
                                                               aylo::plugins::input::ReadActionArguments{"boost"});
    EXPECT_FLOAT_EQ(boost.value.x, -2.0F);
}

TEST(InputCalls, UnknownNamesThrow) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    EXPECT_THROW(call<void>(app, aylo::plugins::input::kAddActionCall,
                            aylo::plugins::input::AddActionArguments{.name = "shoot", .type = "trigger"}),
                 aylo::plugins::input::exceptions::InputException);
    EXPECT_THROW(
        call<void>(app, aylo::plugins::input::kAddActionCall, aylo::plugins::input::AddActionArguments{.name = ""}),
        aylo::plugins::input::exceptions::InputException);
    call<void>(app, aylo::plugins::input::kAddActionCall, aylo::plugins::input::AddActionArguments{.name = "shoot"});
    try {
        call<void>(app, aylo::plugins::input::kBindCall,
                   aylo::plugins::input::BindArguments{.action = "shoot", .control = "Jump"});
        ADD_FAILURE() << "an unknown control should throw";
    } catch (const aylo::plugins::input::exceptions::InputException& error) {
        EXPECT_NE(std::string{error.what()}.find("'Jump'"), std::string::npos) << error.what();
    }
    EXPECT_THROW(static_cast<void>(call<aylo::plugins::input::ActionState>(
                     app, aylo::plugins::input::kReadActionCall, aylo::plugins::input::ReadActionArguments{"fly"})),
                 aylo::plugins::input::exceptions::InputException);
}

// ---- sprite ----

TEST(SpriteCalls, CreateAtlasCutsAGridNamedAfterItsTexture) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::sprite::SpritePlugin{});
    const auto created = call<aylo::plugins::sprite::AtlasResult>(
        app, aylo::plugins::sprite::kCreateAtlasCall,
        aylo::plugins::sprite::CreateAtlasArguments{
            .texture = "ships.png", .frameSize = {10.0F, 10.0F}, .count = 4, .spacing = {2.0F, 0.0F}, .linear = true});

    EXPECT_EQ(created.atlas, AssetId::fromPath("ships.png"));
    const auto& atlas = atlasOf(app, created.atlas);
    ASSERT_EQ(atlas.getFrames().size(), 4U);
    EXPECT_FLOAT_EQ(atlas.getFrames().at(1).source.getX(), 12.0F);
    EXPECT_EQ(atlas.getTextureDesc().filter, aylo::contract::renderer::TextureFilter::kLinear);
}

TEST(SpriteCalls, AddFrameAndAddAnimationFillAnAtlas) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::sprite::SpritePlugin{});
    const AssetId laser = call<aylo::plugins::sprite::AtlasResult>(
                              app, aylo::plugins::sprite::kCreateAtlasCall,
                              aylo::plugins::sprite::CreateAtlasArguments{.name = "laser", .texture = "laser.png"})
                              .atlas;
    EXPECT_EQ(laser, AssetId::fromPath("laser"));
    EXPECT_TRUE(atlasOf(app, laser).getFrames().empty());

    const auto first = call<aylo::plugins::sprite::FrameResult>(
        app, aylo::plugins::sprite::kAddFrameCall,
        aylo::plugins::sprite::AddFrameArguments{.atlas = laser, .position = {108.0F, 151.0F}, .size = {80.0F, 16.0F}});
    const auto second = call<aylo::plugins::sprite::FrameResult>(
        app, aylo::plugins::sprite::kAddFrameCall,
        aylo::plugins::sprite::AddFrameArguments{
            .atlas = laser, .name = "tip", .size = {8.0F, 8.0F}, .origin = {0.0F, 0.0F}});
    EXPECT_EQ(first.frame, 0U);
    EXPECT_EQ(second.frame, 1U);
    const auto& frames = atlasOf(app, laser).getFrames();
    EXPECT_EQ(frames.at(0).name, "0");
    EXPECT_FLOAT_EQ(frames.at(0).origin.x, 40.0F);  // not given: the centre
    EXPECT_EQ(frames.at(1).name, "tip");
    EXPECT_FLOAT_EQ(frames.at(1).origin.x, 0.0F);

    call<void>(app, aylo::plugins::sprite::kAddAnimationCall,
               aylo::plugins::sprite::AddAnimationArguments{
                   .atlas = laser, .name = "both", .count = 2, .frameDuration = 0.08F, .direction = "pingPong"});
    call<void>(app, aylo::plugins::sprite::kAddAnimationCall,
               aylo::plugins::sprite::AddAnimationArguments{.atlas = laser, .name = "rest", .first = 1, .plays = 1});
    const auto* both = atlasOf(app, laser).findAnimation(aylo::plugins::sprite::animationId("both"));
    ASSERT_NE(both, nullptr);
    EXPECT_EQ(both->frames, (std::vector<std::uint32_t>{0, 1}));
    EXPECT_EQ(both->direction, aylo::plugins::sprite::AnimationDirection::kPingPong);
    const auto* rest = atlasOf(app, laser).findAnimation(aylo::plugins::sprite::animationId("rest"));
    ASSERT_NE(rest, nullptr);
    EXPECT_EQ(rest->frames, (std::vector<std::uint32_t>{1}));  // count 0: up to the last frame
}

TEST(SpriteCalls, MistakesThrowAnAtlasException) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::sprite::SpritePlugin{});
    EXPECT_THROW(static_cast<void>(call<aylo::plugins::sprite::AtlasResult>(
                     app, aylo::plugins::sprite::kCreateAtlasCall, aylo::plugins::sprite::CreateAtlasArguments{})),
                 aylo::plugins::sprite::exceptions::AtlasException);
    EXPECT_THROW(static_cast<void>(call<aylo::plugins::sprite::FrameResult>(
                     app, aylo::plugins::sprite::kAddFrameCall,
                     aylo::plugins::sprite::AddFrameArguments{.atlas = AssetId::fromPath("nowhere")})),
                 aylo::plugins::sprite::exceptions::AtlasException);
    const AssetId sheet =
        call<aylo::plugins::sprite::AtlasResult>(
            app, aylo::plugins::sprite::kCreateAtlasCall,
            aylo::plugins::sprite::CreateAtlasArguments{.texture = "sheet.png", .frameSize = {4.0F, 4.0F}, .count = 2})
            .atlas;
    EXPECT_THROW(call<void>(app, aylo::plugins::sprite::kAddAnimationCall,
                            aylo::plugins::sprite::AddAnimationArguments{
                                .atlas = sheet, .name = "spin", .direction = "sideways"}),
                 aylo::plugins::sprite::exceptions::AtlasException);
    EXPECT_THROW(call<void>(app, aylo::plugins::sprite::kAddAnimationCall,
                            aylo::plugins::sprite::AddAnimationArguments{.atlas = sheet, .name = "far", .first = 5}),
                 aylo::plugins::sprite::exceptions::AtlasException);
}

// ---- particle ----

namespace {
/// @brief Adds the ParticlePlugin and the plugins it requires to @p app, with no backend.
void addParticlePlugins(aylo::ecs::App& app) {
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{std::make_unique<aylo::plugins::graphics::BackendRegistry>(),
                                                          aylo::core::config::Settings{}});
    app.addPlugin(aylo::plugins::shader::ShaderPlugin{});
    app.addPlugin(aylo::plugins::particle::ParticlePlugin{});
}
}  // namespace

TEST(ParticleCalls, ADraftFilledByTheCallsBecomesAnEffect) {
    namespace particle = aylo::plugins::particle;
    aylo::ecs::App app;
    addParticlePlugins(app);
    call<void>(app, particle::kBeginEffectCall,
               particle::BeginEffectArguments{.name = "sparks",
                                              .simulation = "cpu",
                                              .space = "local",
                                              .loop = false,
                                              .shape = "circle",
                                              .radius = 4.0F,
                                              .edgeOnly = true,
                                              .lifetimeMin = 0.25F,
                                              .lifetimeMax = 0.5F,
                                              .texture = AssetId::fromPath("spark.png"),
                                              .blend = "alpha",
                                              .alignment = "velocity",
                                              .flipbookColumns = 4});
    call<void>(app, particle::kAddBurstCall, particle::AddBurstArguments{.name = "sparks", .count = 16});
    call<void>(app, particle::kAddColorKeyCall,
               particle::AddColorKeyArguments{.name = "sparks", .t = 0.0F, .color = {1.0F, 1.0F, 0.0F, 1.0F}});
    call<void>(app, particle::kAddColorKeyCall,
               particle::AddColorKeyArguments{.name = "sparks", .t = 1.0F, .color = {1.0F, 0.0F, 0.0F, 0.0F}});
    call<void>(app, particle::kAddCurveKeyCall,
               particle::AddCurveKeyArguments{.name = "sparks", .curve = "speed", .t = 1.0F, .value = 0.0F});
    const auto added =
        call<particle::EffectResult>(app, particle::kAddEffectCall, particle::AddEffectArguments{.name = "sparks"});

    EXPECT_EQ(added.effect, AssetId::fromPath("sparks"));
    const auto& resources = app.getWorld().getResources();
    const particle::ParticleEffect* effect = resources.require<particle::ParticleEffectAssets>().get(added.effect);
    ASSERT_NE(effect, nullptr);
    EXPECT_EQ(effect->desc.simulation, aylo::contract::renderer::ParticleSimulation::kCpu);
    EXPECT_EQ(effect->desc.space, aylo::contract::renderer::ParticleSpace::kLocal);
    EXPECT_FALSE(effect->desc.loop);
    const auto* circle = std::get_if<aylo::contract::renderer::CircleShape>(&effect->desc.shape);
    ASSERT_NE(circle, nullptr);
    EXPECT_FLOAT_EQ(circle->radius, 4.0F);
    EXPECT_TRUE(circle->edgeOnly);
    EXPECT_FLOAT_EQ(effect->desc.lifetime.max, 0.5F);
    EXPECT_FLOAT_EQ(effect->desc.size.min, 1.0F);  // not given: the default
    ASSERT_EQ(effect->desc.bursts.size(), 1U);
    EXPECT_EQ(effect->desc.bursts[0].count, 16U);
    ASSERT_EQ(effect->desc.colorOverLife.keys.size(), 2U);
    EXPECT_FLOAT_EQ(effect->desc.colorOverLife.keys[1].value.w, 0.0F);
    EXPECT_TRUE(effect->desc.sizeOverLife.keys.empty());
    ASSERT_EQ(effect->desc.speedOverLife.keys.size(), 1U);
    EXPECT_EQ(effect->texture, AssetId::fromPath("spark.png"));
    EXPECT_EQ(effect->blend, aylo::contract::renderer::BlendMode::kAlpha);
    EXPECT_EQ(effect->alignment, aylo::contract::renderer::ParticleAlignment::kVelocity);
    EXPECT_EQ(effect->flipbook.columns, 4U);
    EXPECT_TRUE(resources.require<particle::ParticleEffectDrafts>().effects.empty());
}

TEST(ParticleCalls, MistakesThrowAParticleException) {
    namespace particle = aylo::plugins::particle;
    using aylo::contract::renderer::exceptions::ParticleException;
    aylo::ecs::App app;
    addParticlePlugins(app);
    EXPECT_THROW(call<void>(app, particle::kBeginEffectCall, particle::BeginEffectArguments{}), ParticleException);
    EXPECT_THROW(call<void>(app, particle::kBeginEffectCall, particle::BeginEffectArguments{.name = "a", .blend = "x"}),
                 ParticleException);
    EXPECT_THROW(
        call<void>(app, particle::kBeginEffectCall, particle::BeginEffectArguments{.name = "a", .flipbookRows = 0}),
        ParticleException);
    EXPECT_THROW(call<void>(app, particle::kAddBurstCall, particle::AddBurstArguments{.name = "never", .count = 1}),
                 ParticleException);

    call<void>(app, particle::kBeginEffectCall, particle::BeginEffectArguments{.name = "a"});
    EXPECT_THROW(call<void>(app, particle::kAddCurveKeyCall, particle::AddCurveKeyArguments{.name = "a", .curve = "x"}),
                 ParticleException);
    call<void>(app, particle::kAddCurveKeyCall, particle::AddCurveKeyArguments{.name = "a", .t = 2.0F});
    try {
        static_cast<void>(
            call<particle::EffectResult>(app, particle::kAddEffectCall, particle::AddEffectArguments{.name = "a"}));
        ADD_FAILURE() << "an effect with a key at t = 2 was added";
    } catch (const ParticleException& error) {
        EXPECT_NE(std::string_view{error.what()}.find("Particle effect 'a'"), std::string_view::npos) << error.what();
    }
    // The invalid draft is gone: it does not come back with the next try.
    EXPECT_THROW(static_cast<void>(call<particle::EffectResult>(app, particle::kAddEffectCall,
                                                                particle::AddEffectArguments{.name = "a"})),
                 ParticleException);
    EXPECT_EQ(app.getWorld().getResources().require<particle::ParticleEffectAssets>().getCount(), 0U);
}

// ---- texture, font, tilemap ----

TEST(TextureCalls, LoadTextureAsksForTheImageWithItsSampling) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    const auto state = std::make_shared<aylo::test::plugins::FakeBackendState>();
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{aylo::test::plugins::makeFakeRegistry(state),
                                                          aylo::test::plugins::makeFakeSettings()});
    const auto loaded = call<aylo::plugins::graphics::TextureResult>(
        app, aylo::plugins::graphics::kLoadTextureCall,
        aylo::plugins::graphics::LoadTextureArguments{.path = "planet.png", .linear = true, .wrap = true});

    EXPECT_EQ(loaded.texture, AssetId::fromPath("planet.png"));
    const auto* entry =
        app.getWorld().getResources().require<aylo::plugins::graphics::TextureAssets>().get(loaded.texture);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->desc.filter, aylo::contract::renderer::TextureFilter::kLinear);
    EXPECT_TRUE(entry->desc.repeat);
}

TEST(TextCalls, LoadFontAsksForTheFont) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    const auto state = std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state),
                                           aylo::test::plugins::makeFakeSettings());
    const auto loaded = call<aylo::plugins::text::FontResult>(
        app, aylo::plugins::text::kLoadFontCall, aylo::plugins::text::LoadFontArguments{"missing-font.json"});

    EXPECT_EQ(loaded.font, AssetId::fromPath("missing-font.json"));
    EXPECT_NE(app.getWorld().getResources().require<aylo::plugins::text::FontAssets>().getEntry(loaded.font), nullptr);
}

TEST(TilemapCalls, SpawningAMapThatFailedToLoadSpawnsNothing) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    const auto state = std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state),
                                           aylo::test::plugins::makeFakeSettings());
    const auto loaded = call<aylo::plugins::tilemap::TilemapResult>(
        app, aylo::plugins::tilemap::kLoadTilemapCall, aylo::plugins::tilemap::LoadTilemapArguments{"missing.tmj"});
    EXPECT_EQ(loaded.map, AssetId::fromPath("missing.tmj"));
    EXPECT_NE(app.getWorld().getResources().require<aylo::plugins::tilemap::TilemapAssets>().getEntry(loaded.map),
              nullptr);

    const auto spawned = call<aylo::plugins::tilemap::SpawnTilemapResult>(
        app, aylo::plugins::tilemap::kSpawnTilemapCall, aylo::plugins::tilemap::SpawnTilemapArguments{loaded.map});
    EXPECT_EQ(spawned.layers, 0U);
}

// ---- audio ----

namespace {
/// @brief An App with the AudioPlugin over the fake device.
struct AudioCallsApp {
    std::shared_ptr<aylo::test::plugins::FakeAudioState> state{std::make_shared<aylo::test::plugins::FakeAudioState>()};
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};

    AudioCallsApp() {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::audio::AudioPlugin{aylo::test::plugins::makeFakeAudioRegistry(state),
                                                        aylo::test::plugins::makeFakeAudioSettings()});
    }
};
}  // namespace

TEST(AudioCalls, LoadSoundAsksForTheSound) {
    AudioCallsApp audio;
    const auto loaded = call<aylo::plugins::audio::SoundResult>(
        audio.app, aylo::plugins::audio::kLoadSoundCall,
        aylo::plugins::audio::LoadSoundArguments{.path = "music.ogg", .stream = true});

    EXPECT_EQ(loaded.sound, AssetId::fromPath("music.ogg"));
    const auto* entry =
        audio.app.getWorld().getResources().require<aylo::plugins::audio::AudioAssets>().get(loaded.sound);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->desc.stream);
}

TEST(AudioCalls, PlaySoundSendsAPlaySound) {
    AudioCallsApp audio;
    const AssetId shot = AssetId::fromPath("shot.wav");
    call<void>(audio.app, aylo::plugins::audio::kPlaySoundCall,
               aylo::plugins::audio::PlaySoundArguments{.sound = shot, .bus = "music", .volume = 0.5F});
    call<void>(audio.app, aylo::plugins::audio::kPlaySoundCall,
               aylo::plugins::audio::PlaySoundArguments{.sound = shot, .position = {3.0F, 4.0F}});
    EXPECT_THROW(call<void>(audio.app, aylo::plugins::audio::kPlaySoundCall,
                            aylo::plugins::audio::PlaySoundArguments{.sound = shot, .bus = "voice"}),
                 aylo::plugins::audio::exceptions::AudioCallException);

    aylo::ecs::EventCursor cursor;
    std::vector<aylo::plugins::audio::PlaySound> sent;
    for (const aylo::plugins::audio::PlaySound& sound :
         audio.app.getWorld().getEvents().read<aylo::plugins::audio::PlaySound>(cursor)) {
        sent.push_back(sound);
    }
    ASSERT_EQ(sent.size(), 2U);
    EXPECT_EQ(sent[0].sound, shot);
    EXPECT_EQ(sent[0].bus, aylo::contract::audiodevice::Bus::kMusic);
    EXPECT_FLOAT_EQ(sent[0].volume, 0.5F);
    EXPECT_FALSE(sent[0].position);  // not given: centred
    EXPECT_EQ(sent[1].bus, aylo::contract::audiodevice::Bus::kEffects);
    ASSERT_TRUE(sent[1].position);
    EXPECT_FLOAT_EQ(sent[1].position->y, 4.0F);
}

// ---- physics ----

namespace {
namespace physics = aylo::plugins::physics;

constexpr float kTolerance = 0.05F;
constexpr std::uint32_t kWallLayer = 1U << 1;

/// @brief An App with the physics plugin and a static wall across the +X axis, its left face at x = 90.
struct PhysicsCallsApp {
    aylo::ecs::App app{aylo::ecs::RunMode::kServer};
    aylo::ecs::Entity wall;

    PhysicsCallsApp() {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(physics::PhysicsPlugin{physics::PhysicsSettings{
            .maxBodies = 16, .maxBodyPairs = 16, .maxContactConstraints = 16, .tempAllocatorBytes = 1024 * 1024}});
        const physics::Collider collider{.size = {20.0F, 200.0F}, .layer = kWallLayer};
        const aylo::plugins::transform::Transform at{.position = {100.0F, 0.0F}};
        wall = app.getWorld().spawn(collider, at);
        static_cast<void>(world()->createBody(wall, collider, nullptr, at));
    }

    /// @return The physics world, to give bodies without stepping the App.
    [[nodiscard]] aylo::ecs::Mut<physics::PhysicsWorld> world() {
        return app.getWorld().getResources().requireMut<physics::PhysicsWorld>();
    }
};
}  // namespace

TEST(PhysicsCalls, RaycastHitsTheNearestBodyItsFilterSees) {
    PhysicsCallsApp physics;
    const auto hit =
        call<physics::HitResult>(physics.app, physics::kRaycastCall, physics::RaycastArguments{.maxDistance = 1000.0F});
    ASSERT_TRUE(hit.hit);
    EXPECT_EQ(hit.entity, physics.wall);
    EXPECT_NEAR(hit.point.x, 90.0F, kTolerance);
    EXPECT_NEAR(hit.normal.x, -1.0F, kTolerance);
    EXPECT_NEAR(hit.distance, 90.0F, kTolerance);

    const auto ignored = call<physics::HitResult>(
        physics.app, physics::kRaycastCall, physics::RaycastArguments{.maxDistance = 1000.0F, .ignore = physics.wall});
    EXPECT_FALSE(ignored.hit);
    EXPECT_TRUE(ignored.entity.isNull());
    const auto masked = call<physics::HitResult>(physics.app, physics::kRaycastCall,
                                                 physics::RaycastArguments{.maxDistance = 1000.0F, .mask = 1U << 3});
    EXPECT_FALSE(masked.hit);
}

TEST(PhysicsCalls, ShapeCastAndOverlapUseTheShapeFields) {
    PhysicsCallsApp physics;
    const auto cast = call<physics::HitResult>(
        physics.app, physics::kShapeCastCall,
        physics::ShapeCastArguments{.shape = physics::ShapeType::kCircle, .radius = 10.0F, .maxDistance = 1000.0F});
    ASSERT_TRUE(cast.hit);
    EXPECT_EQ(cast.entity, physics.wall);
    EXPECT_NEAR(cast.distance, 80.0F, kTolerance);  // the circle's edge touches the face at x = 90

    const auto inside = call<physics::OverlapResult>(physics.app, physics::kOverlapCall,
                                                     physics::OverlapArguments{.at = {100.0F, 0.0F}});
    EXPECT_EQ(inside.count, 1U);
    EXPECT_EQ(inside.first, physics.wall);
    const auto outside = call<physics::OverlapResult>(physics.app, physics::kOverlapCall,
                                                      physics::OverlapArguments{.at = {500.0F, 0.0F}});
    EXPECT_EQ(outside.count, 0U);
    EXPECT_TRUE(outside.first.isNull());
    EXPECT_THROW(
        call<physics::OverlapResult>(physics.app, physics::kOverlapCall,
                                     physics::OverlapArguments{.shape = physics::ShapeType::kCircle, .radius = -1.0F}),
        physics::exceptions::PhysicsException);
}

TEST(PhysicsCalls, MoveAndSlideStopsAnEntityAgainstAWall) {
    PhysicsCallsApp physics;
    const aylo::ecs::Entity ship =
        physics.app.getWorld().spawn(physics::Collider{.shape = physics::ShapeType::kCircle, .radius = 10.0F});

    const auto moved = call<physics::MoveAndSlideResult>(
        physics.app, physics::kMoveAndSlideCall,
        physics::MoveAndSlideArguments{.entity = ship, .delta = {200.0F, 0.0F}, .skin = 0.5F});
    EXPECT_NEAR(moved.position.x, 79.5F, kTolerance);  // a skin away from the wall
    EXPECT_NEAR(moved.position.y, 0.0F, kTolerance);

    const aylo::ecs::Entity ghost = physics.app.getWorld().spawn();
    EXPECT_THROW(call<physics::MoveAndSlideResult>(physics.app, physics::kMoveAndSlideCall,
                                                   physics::MoveAndSlideArguments{.entity = ghost}),
                 physics::exceptions::PhysicsException);
}

TEST(PhysicsCalls, PushesAndGravityReachTheWorld) {
    PhysicsCallsApp physics;
    call<void>(physics.app, physics::kSetGravityCall, physics::GravityArguments{.gravity = {0.0F, 0.0F}});
    EXPECT_FLOAT_EQ(physics.world()->getGravity().y, 0.0F);

    const physics::Collider collider{.shape = physics::ShapeType::kCircle, .radius = 5.0F};
    const aylo::plugins::transform::Transform at{.position = {0.0F, 300.0F}};
    const aylo::ecs::Entity ball = physics.app.getWorld().spawn(collider, at, physics::RigidBody{});
    const physics::RigidBody body{.linearDamping = 0.0F};
    static_cast<void>(physics.world()->createBody(ball, collider, &body, at));

    call<void>(physics.app, physics::kApplyImpulseCall, physics::PushArguments{.entity = ball, .value = {10.0F, 0.0F}});
    physics.world()->step(1.0F / 60.0F);
    const auto velocity = physics.world()->getBodyVelocity(ball);
    ASSERT_TRUE(velocity);
    EXPECT_NEAR(velocity->linear.x, 10.0F, kTolerance);  // one kilogram: the impulse is the change of velocity

    call<void>(physics.app, physics::kApplyForceCall, physics::PushArguments{.entity = ball, .value = {0.0F, 60.0F}});
    call<void>(physics.app, physics::kApplyTorqueCall, physics::TorqueArguments{.entity = ball, .torque = 1.0F});
    physics.world()->step(1.0F / 60.0F);
    const auto pushed = physics.world()->getBodyVelocity(ball);
    ASSERT_TRUE(pushed);
    EXPECT_NEAR(pushed->linear.y, 1.0F, kTolerance);  // 60 N for a sixtieth of a second
    EXPECT_GT(pushed->angular, 0.0F);
}
