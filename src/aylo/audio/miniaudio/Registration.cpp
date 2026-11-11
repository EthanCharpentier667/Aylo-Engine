/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#include "audio/miniaudio/Registration.hpp"

#include <cstdint>
#include <memory>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/MiniaudioDevice.hpp"
#include "aylo/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "aylo/contract/audiodevice/IAudioDevice.hpp"
#include "core/config/Settings.hpp"

namespace aylo::audio::miniaudio {

void registerAudio(contract::audiodevice::AudioDeviceRegistry& registry) {
    registry.add("miniaudio",
                 [](const aylo::core::config::Settings& section,
                    std::uint32_t maxVoices) -> std::unique_ptr<contract::audiodevice::IAudioDevice> {
                     return std::make_unique<MiniaudioDevice>(maxVoices, MiniaudioConfig::fromSettings(section));
                 });
}

}  // namespace aylo::audio::miniaudio
