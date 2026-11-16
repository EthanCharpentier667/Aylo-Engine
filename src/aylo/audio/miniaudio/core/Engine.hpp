/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Engine
*/

#pragma once

#include <miniaudio.h>

#include <cstdint>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "aylo/audio/miniaudio/Export.hpp"

namespace aylo::audio::miniaudio::core {
/// @brief Owns a ma_engine: the audio device and the mixer, started on construction and stopped on destruction.
class AYLO_AUDIO_MINIAUDIO_API Engine {
  public:
    /// @param config The sample rate, channels and period size to mix with; 0 lets miniaudio choose.
    /// @throws exceptions::MiniaudioException If the device cannot open.
    explicit Engine(const MiniaudioConfig& config);
    ~Engine();
    Engine(const Engine& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(Engine&& other) = delete;

    /// @brief Stops the device: nothing plays until start(); sounds keep their position.
    void stop();
    /// @brief Restarts the device after stop().
    void start();

    /// @return The engine, for the groups and sounds attached to it.
    [[nodiscard]] ma_engine* get() noexcept;

    /// @return The sample rate the engine mixes at, in Hz.
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;

    /// @return Frames the output has mixed so far: the audio clock, which only the audio thread advances.
    [[nodiscard]] std::uint64_t timeInFrames() const noexcept;

    /// @return The playback buffer of the device, in seconds (its period size times its periods, at its own sample
    /// rate): how long a mixed frame waits before it is heard. 0 when the engine has no device.
    [[nodiscard]] double outputLatency() const noexcept;

    /// @brief Sets the master volume of the engine, affecting all groups and sounds attached to it.
    /// @param volume The volume, 0.0 (silence) to 1.0 (full volume); values above 1.0 are allowed, but may distort.
    void setVolume(float volume);

  private:
    ma_engine _engine{};  ///< Never moves: every group and sound points to it.
};
}  // namespace aylo::audio::miniaudio::core
