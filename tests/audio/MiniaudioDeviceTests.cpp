/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MiniaudioDeviceTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

#include "aylo/audio/miniaudio/Registration.hpp"
#include "aylo/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/contract/audiodevice/IAudioDevice.hpp"
#include "aylo/core/config/Settings.hpp"

using aylo::contract::audiodevice::IAudioDevice;
using aylo::contract::audiodevice::SoundDesc;
using aylo::contract::audiodevice::SoundId;

namespace {
/// @brief Appends @p value to @p bytes, little endian, on @p size bytes.
void put(std::vector<std::byte>& bytes, const std::uint32_t value, const std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        bytes.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFU));
    }
}

/// @return A WAV file of @p frames frames of silence: 16-bit mono PCM at 44100 Hz, as a game's pack holds it.
std::vector<std::byte> silentWav(const std::uint32_t frames) {
    constexpr std::uint32_t kRate = 44100;
    constexpr std::uint32_t kBytesPerFrame = 2;
    const std::uint32_t data = frames * kBytesPerFrame;
    std::vector<std::byte> bytes;
    for (const char c : std::array{'R', 'I', 'F', 'F'}) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    put(bytes, 36 + data, 4);
    for (const char c : std::array{'W', 'A', 'V', 'E', 'f', 'm', 't', ' '}) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    put(bytes, 16, 4);     // the size of the format chunk
    put(bytes, 1, 2);      // PCM
    put(bytes, 1, 2);      // mono
    put(bytes, kRate, 4);  // frames per second
    put(bytes, kRate * kBytesPerFrame, 4);
    put(bytes, kBytesPerFrame, 2);  // bytes per frame
    put(bytes, 16, 2);              // bits per sample
    for (const char c : std::array{'d', 'a', 't', 'a'}) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    put(bytes, data, 4);
    bytes.resize(bytes.size() + data, std::byte{0});
    return bytes;
}

/// @brief A miniaudio device from its registry, as the audio plugin makes it; the test is skipped if none opens.
class MiniaudioDeviceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        aylo::contract::audiodevice::AudioDeviceRegistry registry;
        aylo::audio::miniaudio::registerAudio(registry);
        const auto* factory = registry.find("miniaudio");
        ASSERT_NE(factory, nullptr);
        try {
            _device = (*factory)(aylo::core::config::Settings{}, 8);
        } catch (const std::exception& error) {
            GTEST_SKIP() << "No audio device opens here: " << error.what();
        }
    }

    std::unique_ptr<IAudioDevice> _device;
};
}  // namespace

TEST_F(MiniaudioDeviceTest, PlaysASoundGivenAsBytes) {
    const std::vector<std::byte> wav = silentWav(44100);
    const SoundId sound = _device->createSound(wav, SoundDesc{.stream = false});

    ASSERT_NE(sound.value, 0U);
    EXPECT_TRUE(_device->isPlaying(_device->play(sound, {})));
    _device->destroySound(sound);
}

TEST_F(MiniaudioDeviceTest, StreamsASoundGivenAsBytes) {
    // What a game's pack gives the backend for its music: the resource manager could only stream a file.
    const std::vector<std::byte> wav = silentWav(44100);
    const SoundId sound = _device->createSound(wav, SoundDesc{.stream = true});

    ASSERT_NE(sound.value, 0U);
    const auto first = _device->play(sound, {});
    const auto second = _device->play(sound, {});  // each voice decodes the bytes on its own
    EXPECT_TRUE(_device->isPlaying(first));
    EXPECT_TRUE(_device->isPlaying(second));
    _device->destroySound(sound);
    EXPECT_FALSE(_device->isPlaying(first));
}

TEST_F(MiniaudioDeviceTest, RefusesBytesThatAreNoSound) {
    const std::vector<std::byte> noise(64, std::byte{0x5A});

    EXPECT_ANY_THROW(static_cast<void>(_device->createSound(noise, SoundDesc{.stream = true})));
    EXPECT_ANY_THROW(static_cast<void>(_device->createSound(noise, SoundDesc{.stream = false})));
}
