/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NullAudioDeviceTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <span>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "plugins/audio/NullAudioDevice.hpp"

namespace {
/// @return A sound created from a file path in @p device.
aylo::contract::audiodevice::SoundId loadSound(aylo::plugins::audio::NullAudioDevice& device) {
    return device.createSound(std::filesystem::path{"assets/shot.wav"}, {});
}
}  // namespace

TEST(NullAudioDevice, LoadsEverySound) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::SoundId fromFile = loadSound(device);
    const aylo::contract::audiodevice::SoundId fromMemory = device.createSound(std::span<const std::byte>{}, {});
    EXPECT_NE(fromFile.value, 0U);
    EXPECT_NE(fromMemory.value, 0U);
    EXPECT_NE(fromFile, fromMemory);
}

TEST(NullAudioDevice, EndsAOneShotVoiceAtTheNextUpdate) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::VoiceId voice = device.play(loadSound(device), {});
    EXPECT_TRUE(device.isPlaying(voice));
    device.update();
    EXPECT_FALSE(device.isPlaying(voice));
}

TEST(NullAudioDevice, PlaysALoopingVoiceUntilStopped) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::VoiceId voice = device.play(loadSound(device), {.loop = true});
    device.update();
    device.update();
    EXPECT_TRUE(device.isPlaying(voice));
    device.stop(voice, 0.0F);
    EXPECT_FALSE(device.isPlaying(voice));
}

TEST(NullAudioDevice, KeepsAPausedVoice) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::VoiceId voice = device.play(loadSound(device), {});
    device.setPaused(voice, true);
    device.update();
    EXPECT_TRUE(device.isPlaying(voice));
    device.setPaused(voice, false);
    device.update();
    EXPECT_FALSE(device.isPlaying(voice));
}

TEST(NullAudioDevice, NeverLetsAStaleIdControlANewVoice) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::SoundId sound = loadSound(device);
    const aylo::contract::audiodevice::VoiceId old = device.play(sound, {});
    device.update();
    const aylo::contract::audiodevice::VoiceId reused = device.play(sound, {.loop = true});
    EXPECT_EQ(reused.index, old.index);
    EXPECT_NE(reused.generation, old.generation);
    device.stop(old, 0.0F);
    EXPECT_TRUE(device.isPlaying(reused));
}

TEST(NullAudioDevice, NeverHandsOutAVoiceThatADefaultIdNames) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::VoiceId voice = device.play(loadSound(device), {.loop = true});
    EXPECT_NE(voice, aylo::contract::audiodevice::VoiceId{});
    EXPECT_FALSE(device.isPlaying(aylo::contract::audiodevice::VoiceId{}));
}

TEST(NullAudioDevice, PlaysNothingBeyondItsVoiceCount) {
    aylo::plugins::audio::NullAudioDevice device{2};
    const aylo::contract::audiodevice::SoundId sound = loadSound(device);
    EXPECT_TRUE(device.isPlaying(device.play(sound, {.loop = true})));
    EXPECT_TRUE(device.isPlaying(device.play(sound, {.loop = true})));
    EXPECT_FALSE(device.isPlaying(device.play(sound, {.loop = true})));
}

TEST(NullAudioDevice, PlaysNothingForAnUnknownSound) {
    aylo::plugins::audio::NullAudioDevice device;
    EXPECT_FALSE(device.isPlaying(device.play(aylo::contract::audiodevice::SoundId{.value = 42}, {})));
}

TEST(NullAudioDevice, StopsTheVoicesOfADestroyedSound) {
    aylo::plugins::audio::NullAudioDevice device;
    const aylo::contract::audiodevice::SoundId sound = loadSound(device);
    const aylo::contract::audiodevice::VoiceId voice = device.play(sound, {.loop = true});
    device.destroySound(sound);
    EXPECT_FALSE(device.isPlaying(voice));
    EXPECT_FALSE(device.isPlaying(device.play(sound, {})));
}
