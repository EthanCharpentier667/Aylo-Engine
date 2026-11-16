/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Engine
*/

#include "audio/miniaudio/core/Engine.hpp"

#include <miniaudio.h>

#include <cstdint>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

namespace aylo::audio::miniaudio::core {
Engine::Engine(const MiniaudioConfig& config) {
    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.sampleRate = config.sampleRate;
    engineConfig.channels = config.channels;
    engineConfig.periodSizeInMilliseconds = config.periodSizeInMilliseconds;
    const ma_result result = ma_engine_init(&engineConfig, &_engine);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to initialize the engine", result);
    }
}

Engine::~Engine() { ma_engine_uninit(&_engine); }

ma_engine* Engine::get() noexcept { return &_engine; }

void Engine::stop() { ma_engine_stop(&_engine); }

void Engine::start() { ma_engine_start(&_engine); }

std::uint32_t Engine::sampleRate() const noexcept { return ma_engine_get_sample_rate(&_engine); }

std::uint64_t Engine::timeInFrames() const noexcept { return ma_engine_get_time_in_pcm_frames(&_engine); }

double Engine::outputLatency() const noexcept {
    // ma_engine_get_device() takes a non-const engine but only reads it.
    const ma_device* device = ma_engine_get_device(const_cast<ma_engine*>(&_engine));  // NOLINT(*-const-cast)
    if (device == nullptr || device->playback.internalSampleRate == 0) {
        return 0.0;
    }
    const double frames = static_cast<double>(device->playback.internalPeriodSizeInFrames) *
                          static_cast<double>(device->playback.internalPeriods);
    return frames / static_cast<double>(device->playback.internalSampleRate);
}

void Engine::setVolume(float volume) { ma_engine_set_volume(&_engine, volume); }

}  // namespace aylo::audio::miniaudio::core
