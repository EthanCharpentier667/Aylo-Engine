/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include "aylo/audio/miniaudio/Registration.hpp"
#include "aylo/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "core/library/LibraryEntry.hpp"

AYLO_LIBRARY(registrar) {
    if (auto* registry = registrar.find<aylo::contract::audiodevice::AudioDeviceRegistry>(
            aylo::contract::audiodevice::kAudioDeviceRegistryService)) {
        aylo::audio::miniaudio::registerAudio(*registry);
    }
}
