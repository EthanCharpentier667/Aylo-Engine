/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "FakeAudioDevice.hpp"
#include "aylo/contract/audiodevice/IAudioDevice.hpp"
#include "aylo/contract/audiodevice/exceptions/AudioExceptions.hpp"
#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/NullAudioDevice.hpp"

TEST(AudioRegistry, AlwaysHasTheNullBackend) {
    const aylo::plugins::audio::AudioRegistry registry;
    EXPECT_EQ(registry.devices.getNames(), std::vector<std::string>{"null"});
    aylo::core::config::Settings settings;
    settings.set("audio.backend", "null");
    const std::unique_ptr<aylo::contract::audiodevice::IAudioDevice> device = registry.createDevice(settings);
    EXPECT_NE(dynamic_cast<aylo::plugins::audio::NullAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, ListsTheNamesSorted) {
    aylo::plugins::audio::AudioRegistry registry;
    registry.devices.add("zeta", nullptr);
    registry.devices.add("alpha", nullptr);
    EXPECT_EQ(registry.devices.getNames(), (std::vector<std::string>{"alpha", "null", "zeta"}));
}

TEST(AudioRegistry, CreatesTheBackendTheConfigNames) {
    const auto state = std::make_shared<aylo::test::plugins::FakeAudioState>();
    const auto registry = aylo::test::plugins::makeFakeAudioRegistry(state);
    const std::unique_ptr<aylo::contract::audiodevice::IAudioDevice> device =
        registry->createDevice(aylo::test::plugins::makeFakeAudioSettings());
    EXPECT_NE(dynamic_cast<aylo::test::plugins::FakeAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, HandsTheFactoryItsSectionAndTheVoices) {
    aylo::plugins::audio::AudioRegistry registry;
    double seenRate = 0.0;
    unsigned seenVoices = 0;
    registry.devices.add("custom",
                         [&seenRate, &seenVoices](const aylo::core::config::Settings& section, std::uint32_t maxVoices)
                             -> std::unique_ptr<aylo::contract::audiodevice::IAudioDevice> {
                             seenRate = section.getNumber("sampleRate", 0.0);
                             seenVoices = maxVoices;
                             return std::make_unique<aylo::plugins::audio::NullAudioDevice>();
                         });
    aylo::core::config::Settings settings;
    settings.set("audio.backend", "custom");
    settings.set("audio.maxVoices", 16.0);
    settings.set("custom.sampleRate", 48000.0);
    static_cast<void>(registry.createDevice(settings));
    EXPECT_DOUBLE_EQ(seenRate, 48000.0);
    EXPECT_EQ(seenVoices, 16U);
}

TEST(AudioRegistry, ThrowsWhenTheConfigChoosesNoBackend) {
    const aylo::plugins::audio::AudioRegistry registry;
    EXPECT_THROW(static_cast<void>(registry.createDevice(aylo::core::config::Settings{})),
                 aylo::contract::audiodevice::exceptions::AudioBackendException);
}

TEST(AudioRegistry, ThrowsForAnUnknownBackend) {
    const aylo::plugins::audio::AudioRegistry registry;
    aylo::core::config::Settings settings;
    settings.set("audio.backend", "fmod");
    EXPECT_THROW(static_cast<void>(registry.createDevice(settings)),
                 aylo::contract::audiodevice::exceptions::AudioBackendException);
}

TEST(AudioRegistry, FallsBackToNullWhenTheDeviceCannotOpen) {
    const auto state = std::make_shared<aylo::test::plugins::FakeAudioState>();
    state->failToOpen = true;
    const auto registry = aylo::test::plugins::makeFakeAudioRegistry(state);
    const std::unique_ptr<aylo::contract::audiodevice::IAudioDevice> device =
        registry->createDevice(aylo::test::plugins::makeFakeAudioSettings());
    EXPECT_NE(dynamic_cast<aylo::plugins::audio::NullAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, FallsBackToNullWhenTheFactoryReturnsNothing) {
    aylo::plugins::audio::AudioRegistry registry;
    registry.devices.add(
        "empty", [](const aylo::core::config::Settings& /*section*/, std::uint32_t /*maxVoices*/) { return nullptr; });
    aylo::core::config::Settings settings;
    settings.set("audio.backend", "empty");
    const std::unique_ptr<aylo::contract::audiodevice::IAudioDevice> device = registry.createDevice(settings);
    EXPECT_NE(dynamic_cast<aylo::plugins::audio::NullAudioDevice*>(device.get()), nullptr);
}

TEST(AudioRegistry, ThrowsWhenARequiredDeviceCannotOpen) {
    const auto state = std::make_shared<aylo::test::plugins::FakeAudioState>();
    state->failToOpen = true;
    const auto registry = aylo::test::plugins::makeFakeAudioRegistry(state);
    aylo::core::config::Settings settings = aylo::test::plugins::makeFakeAudioSettings();
    settings.set("audio.required", true);
    EXPECT_THROW(static_cast<void>(registry->createDevice(settings)),
                 aylo::contract::audiodevice::exceptions::AudioBackendException);
}

TEST(AudioRegistry, NeverHidesAConfigMistakeBehindTheFallback) {
    const auto state = std::make_shared<aylo::test::plugins::FakeAudioState>();
    const auto registry = aylo::test::plugins::makeFakeAudioRegistry(state);
    aylo::core::config::Settings settings = aylo::test::plugins::makeFakeAudioSettings();
    settings.set("fake.unknown", true);
    EXPECT_THROW(static_cast<void>(registry->createDevice(settings)),
                 aylo::core::config::exceptions::SettingsException);
}
