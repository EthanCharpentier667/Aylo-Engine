/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioPluginTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <string>
#include <utility>

#include "FakeAudioDevice.hpp"
#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "plugins/AssetLoaders.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioListener.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioResources.hpp"
#include "plugins/audio/AudioSource.hpp"
#include "plugins/audio/AudioVolumes.hpp"
#include "plugins/audio/Spatial3D.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief An App with the AudioPlugin over the fake device.
class AudioApp {
  public:
    explicit AudioApp(aylo::core::config::Settings settings = aylo::test::plugins::makeFakeAudioSettings(),
                      aylo::ecs::RunMode mode = aylo::ecs::RunMode::kClient)
        : _state{std::make_shared<aylo::test::plugins::FakeAudioState>()}, _app{mode} {
        _app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        _app.addPlugin(
            aylo::plugins::audio::AudioPlugin{aylo::test::plugins::makeFakeAudioRegistry(_state), std::move(settings)});
    }
    ~AudioApp() = default;
    AudioApp(const AudioApp& other) = delete;
    AudioApp& operator=(const AudioApp& other) = delete;
    AudioApp(AudioApp&& other) = delete;
    AudioApp& operator=(AudioApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] aylo::test::plugins::FakeAudioState& state() noexcept { return *_state; }

    /// @return The id of a sound asked for under @p path, loaded at the next frame.
    aylo::core::asset::AssetId load(const std::string& path, aylo::contract::audiodevice::SoundDesc desc = {}) {
        return world().getResources().getMut<aylo::plugins::audio::AudioAssets>()->load(path, desc);
    }

    /// @brief Sends @p event, read at the next frame.
    template <typename T>
    void send(T event) {
        world().getEvents().send(std::move(event));
    }

    /// @return An entity with @p source at @p x.
    aylo::ecs::Entity spawnSource(const aylo::plugins::audio::AudioSource& source, float x = 0.0F) {
        const aylo::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, aylo::plugins::transform::Transform2D{.position = {x, 0.0F}});
        world().getComponents().add(entity, source);
        return entity;
    }

    /// @brief Spawns the listener at @p x.
    void spawnListener(float x) {
        const aylo::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, aylo::plugins::transform::Transform2D{.position = {x, 0.0F}});
        world().getComponents().add(entity, aylo::plugins::audio::AudioListener{});
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeAudioState> _state;  ///< Drives the fake device.
    aylo::ecs::App _app;                                          ///< The App under test.
};
}  // namespace

TEST(AudioPlugin, CreatesNothingBeforeTheFirstFrame) {
    AudioApp app;
    ASSERT_TRUE(app.world().getResources().has<aylo::plugins::audio::Audio>());
    EXPECT_EQ(app.world().getResources().get<aylo::plugins::audio::Audio>()->device, nullptr);
    EXPECT_FLOAT_EQ(app.state().masterVolume, -1.0F);
}

TEST(AudioPlugin, RegistersItsTypesAndBringsTheTransformPlugin) {
    AudioApp app;
    EXPECT_TRUE(app.world().getResources().has<aylo::plugins::audio::AudioAssets>());
    EXPECT_TRUE(app.world().getResources().has<aylo::plugins::audio::AudioVolumes>());
    EXPECT_TRUE(app.world().getComponents().find("aylo.AudioSource"));
    EXPECT_TRUE(app.world().getComponents().find("aylo.AudioListener"));
    EXPECT_TRUE(app.app().hasPlugin(aylo::plugins::transform::TransformPlugin::kName));
}

TEST(AudioPlugin, CreatesTheDeviceWithTheVolumesOfTheConfig) {
    aylo::core::config::Settings settings = aylo::test::plugins::makeFakeAudioSettings();
    settings.set("audio.volume.master", 0.5);
    settings.set("audio.volume.music", 0.25);
    AudioApp app{settings};
    app.app().runFrame();
    EXPECT_NE(app.world().getResources().get<aylo::plugins::audio::Audio>()->device, nullptr);
    EXPECT_FLOAT_EQ(app.state().masterVolume, 0.5F);
    EXPECT_FLOAT_EQ(app.state().busVolumes.at(aylo::contract::audiodevice::Bus::kMusic), 0.25F);
    EXPECT_FLOAT_EQ(app.state().busVolumes.at(aylo::contract::audiodevice::Bus::kEffects), 1.0F);
    EXPECT_FLOAT_EQ(app.world().getResources().get<aylo::plugins::audio::AudioVolumes>()->master, 0.5F);
}

TEST(AudioPlugin, OpensNoDeviceOnAServer) {
    AudioApp app{aylo::test::plugins::makeFakeAudioSettings(), aylo::ecs::RunMode::kServer};
    app.app().runFrame();
    EXPECT_EQ(app.world().getResources().get<aylo::plugins::audio::Audio>()->device, nullptr);
    EXPECT_EQ(app.state().updates, 0);
}

TEST(AudioPlugin, UpdatesTheDeviceEveryFrame) {
    AudioApp app;
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().updates, 2);
}

TEST(AudioPlugin, AppliesTheVolumesWhenTheyChange) {
    AudioApp app;
    app.app().runFrame();
    app.world().getResources().getMut<aylo::plugins::audio::AudioVolumes>()->effects = 0.3F;
    app.app().runFrame();
    EXPECT_FLOAT_EQ(app.state().busVolumes.at(aylo::contract::audiodevice::Bus::kEffects), 0.3F);
}

TEST(AudioPlugin, LoadsAndPlaysASoundAskedForInTheSameFrame) {
    AudioApp app;
    const aylo::core::asset::AssetId shot = app.load("shot.wav");
    app.send(aylo::plugins::audio::PlaySound{.sound = shot, .volume = 0.8F, .pitch = 1.2F});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_EQ(app.state().plays[0].sound, (aylo::contract::audiodevice::SoundId{.value = 1}));
    EXPECT_EQ(app.state().plays[0].params.bus, aylo::contract::audiodevice::Bus::kEffects);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.volume, 0.8F);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pitch, 1.2F);
    EXPECT_FALSE(app.state().plays[0].params.loop);
}

TEST(AudioPlugin, DropsASoundThatIsNotLoaded) {
    AudioApp app;
    app.state().failingPaths.insert("missing.wav");
    app.send(aylo::plugins::audio::PlaySound{.sound = app.load("missing.wav")});
    app.send(aylo::plugins::audio::PlaySound{.sound = aylo::core::asset::AssetId::fromPath("never-asked.wav")});
    app.app().runFrame();
    EXPECT_TRUE(app.state().plays.empty());
}

TEST(AudioPlugin, PansASoundFromTheListener) {
    AudioApp app;
    app.spawnListener(100.0F);
    const aylo::core::asset::AssetId shot = app.load("shot.wav");
    app.send(aylo::plugins::audio::PlaySound{.sound = shot, .position = glm::vec2{300.0F, 0.0F}});
    app.send(aylo::plugins::audio::PlaySound{.sound = shot, .position = glm::vec2{-900.0F, 0.0F}});
    app.send(aylo::plugins::audio::PlaySound{.sound = shot});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 3U);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, 0.5F);   // 200 to the right, panDistance 400
    EXPECT_FLOAT_EQ(app.state().plays[1].params.pan, -1.0F);  // clamped
    EXPECT_FLOAT_EQ(app.state().plays[2].params.pan, 0.0F);   // no position: centred
}

TEST(AudioPlugin, CentresEverySoundWithoutAListener) {
    AudioApp app;
    app.send(aylo::plugins::audio::PlaySound{.sound = app.load("shot.wav"), .position = glm::vec2{300.0F, 0.0F}});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, 0.0F);
}

TEST(AudioPlugin, PlaysTheMusicLoopingOnTheMusicBus) {
    AudioApp app;
    const aylo::core::asset::AssetId music = app.load("music.ogg", {.stream = true});
    app.send(aylo::plugins::audio::PlayMusic{.sound = music, .crossfade = 2.0F});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_EQ(app.state().plays[0].params.bus, aylo::contract::audiodevice::Bus::kMusic);
    EXPECT_TRUE(app.state().plays[0].params.loop);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.fadeIn, 2.0F);
}

TEST(AudioPlugin, CrossfadesFromTheCurrentMusic) {
    AudioApp app;
    const aylo::core::asset::AssetId first = app.load("first.ogg");
    const aylo::core::asset::AssetId second = app.load("second.ogg");
    app.send(aylo::plugins::audio::PlayMusic{.sound = first});
    app.app().runFrame();
    app.send(aylo::plugins::audio::PlayMusic{.sound = second, .crossfade = 3.0F});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 2U);
    ASSERT_EQ(app.state().stops.size(), 1U);
    EXPECT_EQ(app.state().stops[0].first, app.state().plays[0].voice);
    EXPECT_FLOAT_EQ(app.state().stops[0].second, 3.0F);
    EXPECT_TRUE(app.state().playing.contains(app.state().plays[1].voice.index));
}

TEST(AudioPlugin, StopsTheMusic) {
    AudioApp app;
    app.send(aylo::plugins::audio::PlayMusic{.sound = app.load("music.ogg")});
    app.app().runFrame();
    app.send(aylo::plugins::audio::StopMusic{.fadeOut = 0.5F});
    app.app().runFrame();
    ASSERT_EQ(app.state().stops.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().stops[0].second, 0.5F);
    EXPECT_TRUE(app.state().playing.empty());
}

TEST(AudioPlugin, LetsAPlayMusicSentWithAStopMusicWin) {
    AudioApp app;
    const aylo::core::asset::AssetId music = app.load("music.ogg");
    app.send(aylo::plugins::audio::PlayMusic{.sound = music});
    app.send(aylo::plugins::audio::StopMusic{});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_TRUE(app.state().playing.contains(app.state().plays[0].voice.index));
}

TEST(AudioPlugin, ForgetsAMusicThatCannotLoad) {
    AudioApp app;
    app.state().failingPaths.insert("missing.ogg");
    app.send(aylo::plugins::audio::PlayMusic{.sound = app.load("missing.ogg")});
    app.app().runFrame();
    EXPECT_TRUE(app.state().plays.empty());
    EXPECT_FALSE(app.world().getResources().get<aylo::plugins::audio::Audio>()->pendingMusic);
}

TEST(AudioPlugin, StartsASourceOnceItsSoundIsLoaded) {
    AudioApp app;
    app.spawnSource({.sound = app.load("engine.ogg"), .volume = 0.6F, .loop = true});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_TRUE(app.state().plays[0].params.loop);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.volume, 0.6F);
    app.app().runFrame();
    EXPECT_EQ(app.state().plays.size(), 1U);
}

TEST(AudioPlugin, PansASourceFromItsPositionInTheFrameItSpawns) {
    AudioApp app;
    app.spawnListener(0.0F);
    app.spawnSource({.sound = app.load("engine.ogg"), .loop = true}, -200.0F);
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, -0.5F);
}

TEST(AudioPlugin, WaitsForTheWorldPositionOfANewChildBeforePanningIt) {
    AudioApp app;
    app.spawnListener(0.0F);
    const aylo::ecs::Entity parent = app.world().spawn();
    app.world().getComponents().add(parent, aylo::plugins::transform::Transform2D{.position = {400.0F, 0.0F}});
    const aylo::ecs::Entity child = app.spawnSource({.sound = app.load("engine.ogg"), .loop = true}, -200.0F);
    EXPECT_TRUE(app.world().setParent(child, parent));
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, 0.0F);  // its Transform2D alone would say -0.5
    app.app().runFrame();
    EXPECT_FLOAT_EQ(app.state().pans.at(0), 0.5F);
}

TEST(AudioPlugin, FollowsAMovingSource) {
    AudioApp app;
    app.spawnListener(0.0F);
    const aylo::ecs::Entity ship = app.spawnSource({.sound = app.load("engine.ogg"), .loop = true});
    app.app().runFrame();
    app.world().getComponents().getMut<aylo::plugins::transform::Transform2D>(ship)->position.x = 400.0F;
    app.world().getComponents().getMut<aylo::plugins::audio::AudioSource>(ship)->volume = 0.2F;
    app.app().runFrame();
    EXPECT_FLOAT_EQ(app.state().pans.at(0), 1.0F);
    EXPECT_FLOAT_EQ(app.state().volumes.at(0), 0.2F);
}

TEST(AudioPlugin, CentresANonSpatialSource) {
    AudioApp app;
    app.spawnListener(0.0F);
    app.spawnSource({.sound = app.load("engine.ogg"), .loop = true, .spatial = false}, 400.0F);
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, 0.0F);
}

TEST(AudioPlugin, RestartsASourceWhoseSoundChanged) {
    AudioApp app;
    const aylo::core::asset::AssetId idle = app.load("idle.ogg");
    const aylo::core::asset::AssetId boost = app.load("boost.ogg");
    const aylo::ecs::Entity ship = app.spawnSource({.sound = idle, .loop = true});
    app.app().runFrame();
    app.world().getComponents().getMut<aylo::plugins::audio::AudioSource>(ship)->sound = boost;
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 2U);
    ASSERT_EQ(app.state().stops.size(), 1U);
    EXPECT_EQ(app.state().stops[0].first, app.state().plays[0].voice);
    EXPECT_EQ(app.state().plays[1].sound, (aylo::contract::audiodevice::SoundId{.value = 2}));
}

TEST(AudioPlugin, StopsASourceWhenItsComponentIsRemoved) {
    AudioApp app;
    const aylo::ecs::Entity ship = app.spawnSource({.sound = app.load("engine.ogg"), .loop = true});
    app.app().runFrame();
    ASSERT_TRUE(app.world().getComponents().remove<aylo::plugins::audio::AudioSource>(ship));
    app.app().runFrame();
    ASSERT_EQ(app.state().stops.size(), 1U);
    EXPECT_TRUE(app.state().playing.empty());
}

TEST(AudioPlugin, StopsASourceWhenItsEntityIsDespawned) {
    AudioApp app;
    const aylo::ecs::Entity ship = app.spawnSource({.sound = app.load("engine.ogg"), .loop = true});
    app.app().runFrame();
    ASSERT_TRUE(app.world().despawn(ship));
    app.app().runFrame();
    ASSERT_EQ(app.state().stops.size(), 1U);
    EXPECT_TRUE(app.state().playing.empty());
}

TEST(AudioPlugin, PlaysAOneShotSourceOnce) {
    AudioApp app;
    app.spawnSource({.sound = app.load("beep.wav")});
    app.app().runFrame();
    app.state().playing.clear();  // the voice ended
    app.app().runFrame();
    EXPECT_EQ(app.state().plays.size(), 1U);
}

TEST(AudioPlugin, AppliesTheVolumesOnceWhenNothingChanges) {
    AudioApp app;
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().masterVolumeCalls, 1);
}

TEST(AudioPlugin, StopsNoMusicBeforeTheFirstOne) {
    AudioApp app;
    app.send(aylo::plugins::audio::StopMusic{});
    app.send(aylo::plugins::audio::PlayMusic{.sound = app.load("music.ogg")});
    app.app().runFrame();
    EXPECT_EQ(app.state().stopCalls, 0);
}

TEST(AudioPlugin, PlaysAScheduledLoopingFilteredVoice) {
    AudioApp app;
    const aylo::core::asset::AssetId kick = app.load("kick.wav");
    app.send(
        aylo::plugins::audio::PlaySound{.sound = kick, .loop = true, .startAt = 4.25, .lowpass = 800.0F, .voice = 1});
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    const auto& params = app.state().plays[0].params;
    EXPECT_TRUE(params.loop);
    EXPECT_DOUBLE_EQ(params.startAt, 4.25);
    EXPECT_FLOAT_EQ(params.lowpass, 800.0F);
}

TEST(AudioPlugin, ControlsAScriptVoiceByItsId) {
    AudioApp app;
    app.send(aylo::plugins::audio::PlaySound{.sound = app.load("kick.wav"), .loop = true, .voice = 7});
    app.app().runFrame();

    app.send(aylo::plugins::audio::ControlVoice{.voice = 7, .volume = 0.4F, .lowpass = 500.0F});
    app.app().runFrame();
    EXPECT_FLOAT_EQ(app.state().volumes.at(0), 0.4F);
    EXPECT_FLOAT_EQ(app.state().lowpasses.at(0), 500.0F);

    app.send(aylo::plugins::audio::ControlVoice{.voice = 7, .stop = true, .fadeOut = 0.5F});
    app.app().runFrame();
    ASSERT_EQ(app.state().stops.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().stops[0].second, 0.5F);
    EXPECT_FALSE(app.state().playing.contains(0));
}

TEST(AudioPlugin, IgnoresAControlForAVoiceThatIsUnknownOrEnded) {
    AudioApp app;
    app.send(aylo::plugins::audio::PlaySound{.sound = app.load("kick.wav"), .voice = 3});
    app.app().runFrame();
    app.state().playing.clear();  // the voice ended
    app.app().runFrame();         // the plugin forgets it

    app.send(aylo::plugins::audio::ControlVoice{.voice = 3, .volume = 0.1F});
    app.send(aylo::plugins::audio::ControlVoice{.voice = 99, .volume = 0.1F, .stop = true});
    app.app().runFrame();
    EXPECT_TRUE(app.state().volumes.empty());
    EXPECT_EQ(app.state().stopCalls, 0);
}

namespace {
/// @brief Spawns a 3D listener at the origin, facing -Z, turned @p yaw radians to its left.
aylo::ecs::Entity spawnListener3D(AudioApp& app, float yaw = 0.0F) {
    const aylo::ecs::Entity entity = app.world().spawn();
    app.world().getComponents().add(entity, aylo::plugins::transform::Transform3D{
                                                .rotation = {0.0F, std::sin(yaw / 2.0F), 0.0F, std::cos(yaw / 2.0F)}});
    app.world().getComponents().add(entity, aylo::plugins::audio::AudioListener{});
    return entity;
}

/// @return A PlaySound placed at @p position in 3D, 2 m to 40 m.
aylo::plugins::audio::PlaySound placed(aylo::core::asset::AssetId sound, glm::vec3 position, float volume = 1.0F,
                                       std::uint32_t voice = 0) {
    return aylo::plugins::audio::PlaySound{
        .sound = sound,
        .volume = volume,
        .place3D = aylo::plugins::audio::SoundPlace3D{.position = position, .minDistance = 2.0F, .maxDistance = 40.0F},
        .voice = voice};
}
}  // namespace

TEST(AudioPlugin, PansAndAttenuatesA3DSoundForThe3DListener) {
    AudioApp app;
    spawnListener3D(app);
    const aylo::core::asset::AssetId shot = app.load("shot.wav");
    app.send(placed(shot, {4.0F, 0.0F, 0.0F}, 0.8F));  // 4 m on its right
    app.send(placed(shot, {0.0F, 0.0F, -1.0F}));       // 1 m in front
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 2U);
    EXPECT_NEAR(app.state().plays[0].params.pan, 1.0F, 1e-4F);
    EXPECT_NEAR(app.state().plays[0].params.volume, 0.4F, 1e-4F);  // 0.8, halved at twice the minimum distance
    EXPECT_NEAR(app.state().plays[1].params.pan, 0.0F, 1e-4F);
    EXPECT_NEAR(app.state().plays[1].params.volume, 1.0F, 1e-4F);
}

TEST(AudioPlugin, Keeps3DVoicesPannedAsTheListenerTurns) {
    AudioApp app;
    const aylo::ecs::Entity listener = spawnListener3D(app);
    app.send(placed(app.load("drone.wav"), {0.0F, 0.0F, -4.0F}));  // in front
    app.app().runFrame();
    EXPECT_NEAR(app.state().plays[0].params.pan, 0.0F, 1e-4F);
    // A quarter turn to the left: the sound is now on its right.
    app.world().getComponents().getMut<aylo::plugins::transform::Transform3D>(listener)->rotation = {
        0.0F, std::sin(0.785398F), 0.0F, std::cos(0.785398F)};
    app.app().runFrame();
    EXPECT_NEAR(app.state().pans.at(0), 1.0F, 1e-4F);
    EXPECT_NEAR(app.state().volumes.at(0), 0.5F, 1e-4F);
}

TEST(AudioPlugin, TheListenerTheScriptSetsWinsOverTheEntity) {
    AudioApp app;
    spawnListener3D(app);  // would hear the sound in front
    app.world().getResources().getMut<aylo::plugins::audio::Audio>()->listener3D = aylo::plugins::audio::ListenerPose3D{
        .position = {0.0F, 0.0F, -4.0F}, .rotation = {0.0F, std::sin(0.785398F), 0.0F, std::cos(0.785398F)}};
    app.send(placed(app.load("shot.wav"), {0.0F, 0.0F, -8.0F}));  // 4 m in front of the set pose, which faces -X
    app.app().runFrame();
    EXPECT_NEAR(app.state().plays[0].params.pan, 1.0F, 1e-4F);
    EXPECT_NEAR(app.state().plays[0].params.volume, 0.5F, 1e-4F);
}

TEST(AudioPlugin, PlaysA3DSoundCentredAndFullWithoutA3DListener) {
    AudioApp app;
    app.spawnListener(0.0F);  // a 2D listener does not hear in 3D
    app.send(placed(app.load("shot.wav"), {30.0F, 0.0F, 0.0F}, 0.7F));
    app.app().runFrame();
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, 0.0F);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.volume, 0.7F);
}

TEST(AudioPlugin, AScriptVolumeOfA3DVoiceIsAttenuatedToo) {
    AudioApp app;
    spawnListener3D(app);
    app.send(placed(app.load("shot.wav"), {4.0F, 0.0F, 0.0F}, 1.0F, 7));
    app.app().runFrame();
    app.send(aylo::plugins::audio::ControlVoice{.voice = 7, .volume = 0.6F});
    app.app().runFrame();
    EXPECT_NEAR(app.state().volumes.at(0), 0.3F, 1e-4F);  // its new volume, halved by the distance
}

TEST(AudioPlugin, ForgetsA3DVoiceThatEnded) {
    AudioApp app;
    spawnListener3D(app);
    app.send(placed(app.load("shot.wav"), {4.0F, 0.0F, 0.0F}));
    app.app().runFrame();
    app.state().playing.clear();  // it ended
    app.app().runFrame();
    EXPECT_TRUE(app.world().getResources().get<aylo::plugins::audio::Audio>()->placed.empty());
}

TEST(AudioPlugin, LoadsTheSoundsOfTheAssetLoaders) {
    // What a capsule names reaches the audio plugin through the AssetLoaders, as engine.assets.sound would (#157).
    AudioApp app;
    const auto* loaders = app.world().getResources().get<aylo::plugins::AssetLoaders>();
    ASSERT_NE(loaders, nullptr);
    const auto* sounds = app.world().getResources().get<aylo::plugins::audio::AudioAssets>();

    for (const char* path : {"sfx/shot.wav", "sfx/shot.mp3", "music/theme.ogg", "sfx/SHOT.FLAC"}) {
        EXPECT_TRUE(loaders->load(path)) << path;
        const auto* entry = sounds->get(aylo::core::asset::AssetId::fromPath(path));
        ASSERT_NE(entry, nullptr) << path;
        EXPECT_FALSE(entry->desc.stream) << path;
    }
}

TEST(AudioPlugin, KeepsTheStreamOfASoundAScriptLoadedFirst) {
    AudioApp app;
    const aylo::core::asset::AssetId music = app.load("music/theme.ogg", {.stream = true});
    const auto* loaders = app.world().getResources().get<aylo::plugins::AssetLoaders>();
    ASSERT_NE(loaders, nullptr);

    EXPECT_TRUE(loaders->load("music/theme.ogg"));

    const auto* entry = app.world().getResources().get<aylo::plugins::audio::AudioAssets>()->get(music);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->desc.stream);
}
