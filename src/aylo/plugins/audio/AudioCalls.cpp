/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioCalls
*/

#include "plugins/audio/AudioCalls.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/system/EventWriter.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioResources.hpp"
#include "plugins/audio/exceptions/AudioCallException.hpp"

namespace aylo::plugins::audio {
namespace {
/// @return The bus named @p name.
/// @throws exceptions::AudioCallException If no bus has this name.
[[nodiscard]] contract::audiodevice::Bus requireBus(const std::string_view name) {
    if (name == "effects") {
        return contract::audiodevice::Bus::kEffects;
    }
    if (name == "music") {
        return contract::audiodevice::Bus::kMusic;
    }
    throw exceptions::AudioCallException{std::format("Unknown bus '{}' (expected 'effects' or 'music')", name)};
}

SoundResult loadSound(ecs::ResMut<AudioAssets> sounds, const LoadSoundArguments& arguments) {
    const contract::audiodevice::SoundDesc desc{.stream = arguments.stream};
    return SoundResult{.sound = sounds->load(std::filesystem::path{arguments.path.view()}, desc)};
}

VoiceResult playSound(ecs::NonSendMut<Audio> audio, ecs::EventWriter<PlaySound> sounds,
                      const PlaySoundArguments& arguments) {
    PlaySound sound{
        .sound = arguments.sound,
        .bus = requireBus(arguments.bus),
        .volume = arguments.volume,
        .pitch = arguments.pitch,
        .loop = arguments.loop,
        .startAt = arguments.startAt,
        .lowpass = arguments.lowpass,
        .voice = audio->nextVoice++,
    };
    if (!std::isnan(arguments.position.x) && !std::isnan(arguments.position.y)) {
        sound.position = arguments.position;
    }
    sounds.send(sound);
    return VoiceResult{.voice = sound.voice};
}

TimeResult getTime(ecs::NonSend<Audio> audio) {
    return TimeResult{.time = audio->device ? audio->device->getTime() : 0.0};
}

void setVolume(ecs::EventWriter<ControlVoice> controls, const SetVolumeArguments& arguments) {
    controls.send(ControlVoice{.voice = arguments.voice, .volume = arguments.volume});
}

void setFilter(ecs::EventWriter<ControlVoice> controls, const SetFilterArguments& arguments) {
    controls.send(ControlVoice{.voice = arguments.voice, .lowpass = arguments.lowpass});
}

void stopVoice(ecs::EventWriter<ControlVoice> controls, const StopVoiceArguments& arguments) {
    controls.send(ControlVoice{.voice = arguments.voice, .stop = true, .fadeOut = arguments.fadeOut});
}
}  // namespace

void addAudioForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "AudioForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addAudioForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addAudioCalls(ecs::App& app) {
    addAudioForms(app.getPluginRegistry());
    app.addCall(std::string{kLoadSoundCall}, &loadSound);
    app.addCall(std::string{kPlaySoundCall}, &playSound);
    app.addCall(std::string{kGetTimeCall}, &getTime);
    app.addCall(std::string{kSetVolumeCall}, &setVolume);
    app.addCall(std::string{kSetFilterCall}, &setFilter);
    app.addCall(std::string{kStopVoiceCall}, &stopVoice);
}

}  // namespace aylo::plugins::audio
