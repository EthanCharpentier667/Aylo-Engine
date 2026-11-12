/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "aylo/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "aylo/ecs/app/PluginEntry.hpp"
#include "core/config/Settings.hpp"
#include "plugins/audio/AudioCalls.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::audio::addAudioForms(registrar.getPlugins());
    using aylo::plugins::audio::AudioPlugin;
    using aylo::plugins::audio::AudioRegistry;
    auto audio = std::make_shared<std::unique_ptr<AudioRegistry>>(std::make_unique<AudioRegistry>());
    registrar.getLibrary().provide(aylo::contract::audiodevice::kAudioDeviceRegistryService, (*audio)->devices);
    registrar.getLibrary().provideNames(aylo::contract::audiodevice::kAudioDeviceRegistryService, [audio] {
        return *audio ? (*audio)->devices.getNames() : std::vector<std::string>{};
    });
    registrar.getPlugins().add(std::string{AudioPlugin::kName}, [audio](const aylo::core::config::Settings& settings) {
        return std::make_unique<AudioPlugin>(std::move(*audio), settings);
    });
}
