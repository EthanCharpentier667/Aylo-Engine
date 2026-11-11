/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sound
*/

#include "audio/miniaudio/core/Sound.hpp"

#include <miniaudio.h>

#include <cstddef>
#include <format>
#include <memory>
#include <span>
#include <string>

#include "audio/miniaudio/core/Engine.hpp"
#include "audio/miniaudio/core/SoundGroup.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

namespace rtype::audio::miniaudio::core {

Sound::Sound(Engine& engine, const std::string& name, ma_uint32 flags, SoundGroup* group) {
    const ma_result result = ma_sound_init_from_file(engine.get(), name.c_str(), flags,
                                                     group != nullptr ? group->get() : nullptr, nullptr, &_sound);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException(std::format("Failed to load the sound '{}'", name), result);
    }
}

Sound::Sound(Engine& engine, const Sound& model, ma_uint32 flags, SoundGroup* group) {
    const ma_result result =
        ma_sound_init_copy(engine.get(), model.get(), flags, group != nullptr ? group->get() : nullptr, &_sound);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to copy a sound", result);
    }
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
}

Sound::~Sound() {
    ma_sound_uninit(&_sound);
    if (_decoder) {
        ma_decoder_uninit(_decoder.get());
    }
}

ma_sound* Sound::get() noexcept { return &_sound; }

const ma_sound* Sound::get() const noexcept { return &_sound; }

}  // namespace rtype::audio::miniaudio::core
