/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Sound
*/

#include "audio/miniaudio/core/Sound.hpp"

#include <miniaudio.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "audio/miniaudio/core/Engine.hpp"
#include "audio/miniaudio/core/SoundGroup.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

namespace aylo::audio::miniaudio::core {
namespace {
constexpr ma_uint32 kLowpassOrder = 2;  ///< Steepness of the filter: 12 dB per octave.
}  // namespace

Sound::Sound(Engine& engine, const std::string& name, ma_uint32 flags, SoundGroup* group) {
    const ma_result result = ma_sound_init_from_file(engine.get(), name.c_str(), flags,
                                                     group != nullptr ? group->get() : nullptr, nullptr, &_sound);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException(std::format("Failed to load the sound '{}'", name), result);
    }
    remember(engine, group);
}

Sound::Sound(Engine& engine, const Sound& model, ma_uint32 flags, SoundGroup* group) {
    const ma_result result =
        ma_sound_init_copy(engine.get(), model.get(), flags, group != nullptr ? group->get() : nullptr, &_sound);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to copy a sound", result);
    }
    remember(engine, group);
}

Sound::Sound(Engine& engine, std::span<const std::byte> encoded, ma_uint32 flags, SoundGroup* group)
    : _decoder{std::make_unique<ma_decoder>()} {
    // Decoded as the engine mixes, so the sound needs no conversion of its own.
    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, ma_engine_get_channels(engine.get()),
                                                            ma_engine_get_sample_rate(engine.get()));
    ma_result result = ma_decoder_init_memory(encoded.data(), encoded.size(), &config, _decoder.get());
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to decode a sound given in memory", result);
    }
    const ma_uint32 kept = flags & ~static_cast<ma_uint32>(MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_DECODE);
    result = ma_sound_init_from_data_source(engine.get(), _decoder.get(), kept,
                                            group != nullptr ? group->get() : nullptr, &_sound);
    if (result != MA_SUCCESS) {
        ma_decoder_uninit(_decoder.get());
        throw exceptions::MiniaudioException("Failed to play a sound given in memory", result);
    }
    remember(engine, group);
}

void Sound::remember(Engine& engine, SoundGroup* group) noexcept {
    _engine = &engine;
    _output = group != nullptr ? static_cast<ma_node*>(group->get()) : ma_engine_get_endpoint(engine.get());
}

void Sound::setLowpass(float cutoff) {
    if (_engine == nullptr) {
        return;
    }
    // Above this the filter is not a filter: used to open it again, as a node cannot be taken out while it plays.
    const auto rate = static_cast<double>(_engine->sampleRate());
    const double open = rate * 0.49;
    const double wanted = cutoff > 0.0F ? std::min(static_cast<double>(cutoff), open) : open;
    const ma_lpf_node_config config =
        ma_lpf_node_config_init(ma_engine_get_channels(_engine->get()), _engine->sampleRate(), wanted, kLowpassOrder);
    if (_lowpass) {
        const ma_result result = ma_lpf_node_reinit(&config.lpf, _lowpass.get());
        if (result != MA_SUCCESS) {
            throw exceptions::MiniaudioException("Failed to change the filter of a sound", result);
        }
        return;
    }
    if (cutoff <= 0.0F) {
        return;
    }
    auto node = std::make_unique<ma_lpf_node>();
    ma_result result = ma_lpf_node_init(ma_engine_get_node_graph(_engine->get()), &config, nullptr, node.get());
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to create the filter of a sound", result);
    }
    result = ma_node_attach_output_bus(node.get(), 0, _output, 0);
    if (result == MA_SUCCESS) {
        result = ma_node_attach_output_bus(&_sound, 0, node.get(), 0);
    }
    if (result != MA_SUCCESS) {
        ma_lpf_node_uninit(node.get(), nullptr);
        throw exceptions::MiniaudioException("Failed to attach the filter of a sound", result);
    }
    _lowpass = std::move(node);
}

Sound::~Sound() {
    ma_sound_uninit(&_sound);
    if (_lowpass) {
        ma_lpf_node_uninit(_lowpass.get(), nullptr);
    }
    if (_decoder) {
        ma_decoder_uninit(_decoder.get());
    }
}

ma_sound* Sound::get() noexcept { return &_sound; }

const ma_sound* Sound::get() const noexcept { return &_sound; }

}  // namespace aylo::audio::miniaudio::core
