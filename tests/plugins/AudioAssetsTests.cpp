/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioAssetsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "FakeAudioDevice.hpp"
#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/contract/audiodevice/exceptions/AudioExceptions.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/audio/AudioAssets.hpp"

namespace {
/// @brief AudioAssets with a fake device to load them in.
struct AudioAssetsFixture {
    std::shared_ptr<aylo::test::plugins::FakeAudioState> state{std::make_shared<aylo::test::plugins::FakeAudioState>()};
    aylo::test::plugins::FakeAudioDevice device{state};
    aylo::plugins::audio::AudioAssets assets;
};

constexpr std::array<std::byte, 3> kBytes{std::byte{1}, std::byte{2}, std::byte{3}};
}  // namespace

static_assert(aylo::ecs::ResourceTraits<aylo::plugins::audio::AudioAssets>::kName == "aylo.AudioAssets");

TEST(AudioAssets, QueuesASoundAndReturnsTheIdOfItsPath) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load("assets/./sounds/../shot.wav");
    EXPECT_EQ(id, aylo::core::asset::AssetId::fromPath("assets/shot.wav"));
    ASSERT_NE(fixture.assets.get(id), nullptr);
    EXPECT_EQ(fixture.assets.get(id)->path, "assets/shot.wav");
    EXPECT_EQ(fixture.assets.get(id)->state, aylo::plugins::audio::SoundState::kQueued);
    EXPECT_EQ(fixture.assets.getQueued(), std::vector<aylo::core::asset::AssetId>{id});
    EXPECT_FALSE(fixture.assets.getSound(id));
    EXPECT_TRUE(fixture.state->sounds.empty());
}

TEST(AudioAssets, KeepsTheFirstRequestOfAPath) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId first = fixture.assets.load("music.ogg", {.stream = true});
    const aylo::core::asset::AssetId second = fixture.assets.load("music.ogg", {.stream = false});
    EXPECT_EQ(first, second);
    EXPECT_EQ(fixture.assets.getCount(), 1U);
    EXPECT_TRUE(fixture.assets.get(first)->desc.stream);
}

TEST(AudioAssets, LoadsTheQueuedSoundsFromTheirFile) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId shot = fixture.assets.load("shot.wav");
    const aylo::core::asset::AssetId music = fixture.assets.load("music.ogg", {.stream = true});
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    ASSERT_EQ(fixture.state->sounds.size(), 2U);
    EXPECT_EQ(fixture.state->sounds[0].path, "shot.wav");
    EXPECT_EQ(fixture.state->sounds[1].path, "music.ogg");
    EXPECT_TRUE(fixture.state->sounds[1].desc.stream);
    EXPECT_EQ(fixture.assets.getSound(shot), (aylo::contract::audiodevice::SoundId{.value = 1}));
    EXPECT_EQ(fixture.assets.getSound(music), (aylo::contract::audiodevice::SoundId{.value = 2}));
    EXPECT_TRUE(fixture.assets.getQueued().empty());
}

TEST(AudioAssets, LoadsASoundGivenInMemoryAndFreesItsBytes) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load(std::string{"embedded/beep.wav"}, kBytes);
    EXPECT_EQ(fixture.assets.get(id)->encoded.size(), kBytes.size());
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    ASSERT_EQ(fixture.state->sounds.size(), 1U);
    EXPECT_TRUE(fixture.state->sounds[0].path.empty());
    EXPECT_EQ(fixture.state->sounds[0].byteCount, kBytes.size());
    EXPECT_EQ(fixture.assets.get(id)->state, aylo::plugins::audio::SoundState::kLoaded);
    EXPECT_TRUE(fixture.assets.get(id)->encoded.empty());
}

TEST(AudioAssets, MarksASoundThatCannotLoadAsFailed) {
    AudioAssetsFixture fixture;
    fixture.state->failingPaths.insert("missing.wav");
    const aylo::core::asset::AssetId missing = fixture.assets.load("missing.wav");
    const aylo::core::asset::AssetId shot = fixture.assets.load("shot.wav");
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_EQ(fixture.assets.get(missing)->state, aylo::plugins::audio::SoundState::kFailed);
    EXPECT_FALSE(fixture.assets.get(missing)->error.empty());
    EXPECT_FALSE(fixture.assets.getSound(missing));
    EXPECT_TRUE(fixture.assets.getSound(shot));
}

TEST(AudioAssets, DestroysAnUnloadedSoundAtTheNextUpdate) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load("shot.wav");
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    const aylo::contract::audiodevice::SoundId sound = *fixture.assets.getSound(id);
    EXPECT_TRUE(fixture.assets.unload(id));
    EXPECT_EQ(fixture.assets.get(id), nullptr);
    EXPECT_TRUE(fixture.state->destroyed.empty());
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_EQ(fixture.state->destroyed, std::vector<aylo::contract::audiodevice::SoundId>{sound});
}

TEST(AudioAssets, ForgetsAQueuedSoundWithoutLoadingIt) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load("shot.wav");
    EXPECT_TRUE(fixture.assets.unload(id));
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_TRUE(fixture.state->sounds.empty());
    EXPECT_TRUE(fixture.state->destroyed.empty());
}

TEST(AudioAssets, IgnoresAnUnknownId) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId unknown = aylo::core::asset::AssetId::fromPath("unknown.wav");
    EXPECT_FALSE(fixture.assets.unload(unknown));
    fixture.assets.setLoaded(unknown, aylo::contract::audiodevice::SoundId{.value = 7});
    fixture.assets.setFailed(unknown, "no reason");
    EXPECT_EQ(fixture.assets.get(unknown), nullptr);
}

TEST(AudioAssets, LoadsANameAndAPathAsTheSameSound) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId fromPath = fixture.assets.load("sounds/beep.wav");
    const aylo::core::asset::AssetId fromName = fixture.assets.load(std::string{"sounds/beep.wav"}, kBytes);
    EXPECT_EQ(fromPath, fromName);
    EXPECT_EQ(fixture.assets.getCount(), 1U);
}
