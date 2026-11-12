/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#pragma once

#include "aylo/audio/miniaudio/Export.hpp"
#include "aylo/contract/audiodevice/AudioDeviceRegistry.hpp"

namespace aylo::audio::miniaudio {

/// @brief Registers the audio backends of this module: "miniaudio" (MiniaudioDevice), configured by the `miniaudio`
/// section (MiniaudioConfig).
AYLO_AUDIO_MINIAUDIO_API void registerAudio(contract::audiodevice::AudioDeviceRegistry& registry);

}  // namespace aylo::audio::miniaudio
