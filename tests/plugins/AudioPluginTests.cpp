/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioPluginTests
*/

#include <gtest/gtest.h>

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
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioListener.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioResources.hpp"
#include "plugins/audio/AudioSource.hpp"
#include "plugins/audio/AudioVolumes.hpp"
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
        world().getComponents().add(entity, aylo::plugins::transform::Transform{.position = {x, 0.0F}});
        world().getComponents().add(entity, source);
        return entity;
    }

    /// @brief Spawns the listener at @p x.
    void spawnListener(float x) {
        const aylo::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, aylo::plugins::transform::Transform{.position = {x, 0.0F}});
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
    EXPECT_TRUE(app.world().getComponents().find("rtype.AudioSource"));
    EXPECT_TRUE(app.world().getComponents().find("rtype.AudioListener"));
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
    app.world().getComponents().add(parent, aylo::plugins::transform::Transform{.position = {400.0F, 0.0F}});
    const aylo::ecs::Entity child = app.spawnSource({.sound = app.load("engine.ogg"), .loop = true}, -200.0F);
    EXPECT_TRUE(app.world().setParent(child, parent));
    app.app().runFrame();
    ASSERT_EQ(app.state().plays.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().plays[0].params.pan, 0.0F);  // its Transform alone would say -0.5
    app.app().runFrame();
    EXPECT_FLOAT_EQ(app.state().pans.at(0), 0.5F);
}

TEST(AudioPlugin, FollowsAMovingSource) {
    AudioApp app;
    app.spawnListener(0.0F);
    const aylo::ecs::Entity ship = app.spawnSource({.sound = app.load("engine.ogg"), .loop = true});
    app.app().runFrame();
    app.world().getComponents().getMut<aylo::plugins::transform::Transform>(ship)->position.x = 400.0F;
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
