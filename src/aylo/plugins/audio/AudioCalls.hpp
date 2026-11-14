/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioCalls
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep

namespace aylo::plugins::audio {

inline constexpr std::string_view kLoadSoundCall = "aylo.audio.loadSound";  ///< AudioAssets::load().
inline constexpr std::string_view kPlaySoundCall = "aylo.audio.playSound";  ///< Sends a PlaySound.
inline constexpr std::string_view kGetTimeCall = "aylo.audio.getTime";      ///< The audio clock.
inline constexpr std::string_view kSetVolumeCall = "aylo.audio.setVolume";  ///< Sends a ControlVoice.
inline constexpr std::string_view kSetFilterCall = "aylo.audio.setFilter";  ///< Sends a ControlVoice.
inline constexpr std::string_view kStopVoiceCall = "aylo.audio.stopVoice";  ///< Sends a ControlVoice.

/// @brief The arguments of `aylo.audio.loadSound`: asks for a sound file.
struct LoadSoundArguments {
    ecs::String path;    ///< The sound file (WAV, OGG, MP3, FLAC).
    bool stream{false};  ///< Decoded while it plays (music), instead of decoded once into memory (effects).
};
AYLO_ECS_CALL_DATA(LoadSoundArguments, "aylo.audio.LoadSoundArguments", path, stream);

/// @brief The result of `aylo.audio.loadSound`.
struct SoundResult {
    core::asset::AssetId sound;  ///< The sound's id, for `aylo.audio.playSound`, PlayMusic and AudioSource.
};
AYLO_ECS_CALL_DATA(SoundResult, "aylo.audio.SoundResult", sound);

/// @brief The arguments of `aylo.audio.playSound`: a PlaySound, which scripts cannot send as an event (its position
/// is optional).
struct PlaySoundArguments {
    core::asset::AssetId sound;  ///< The sound, from `aylo.audio.loadSound` (or its path).
    ecs::String bus{"effects"};  ///< "effects" or "music".
    float volume{1.0F};          ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};           ///< Playback speed, 1 = unchanged.
    /// @brief The world position to pan from; NaN (not given): centred.
    glm::vec2 position{std::numeric_limits<float>::quiet_NaN()};
    bool loop{false};     ///< Plays until `aylo.audio.stopVoice`.
    double startAt{0.0};  ///< Start time on the clock of `aylo.audio.getTime`, in seconds; 0 or past: at once.
    float lowpass{0.0F};  ///< Low-pass cutoff in Hz; 0: none.
};
AYLO_ECS_CALL_DATA(PlaySoundArguments, "aylo.audio.PlaySoundArguments", sound, bus, volume, pitch, position, loop,
                   startAt, lowpass);

/// @brief The result of `aylo.audio.playSound`.
struct VoiceResult {
    std::uint32_t voice{0};  ///< The voice's id, for setVolume, setFilter and stopVoice; never 0.
};
AYLO_ECS_CALL_DATA(VoiceResult, "aylo.audio.VoiceResult", voice);

/// @brief The result of `aylo.audio.getTime`.
struct TimeResult {
    double time{0.0};  ///< Seconds of the audio device clock; 0 without a device (a server).
};
AYLO_ECS_CALL_DATA(TimeResult, "aylo.audio.TimeResult", time);

/// @brief The arguments of `aylo.audio.setVolume`.
struct SetVolumeArguments {
    std::uint32_t voice{0};  ///< From `aylo.audio.playSound`.
    float volume{1.0F};      ///< Linear gain.
};
AYLO_ECS_CALL_DATA(SetVolumeArguments, "aylo.audio.SetVolumeArguments", voice, volume);

/// @brief The arguments of `aylo.audio.setFilter`.
struct SetFilterArguments {
    std::uint32_t voice{0};  ///< From `aylo.audio.playSound`.
    float lowpass{0.0F};     ///< Cutoff in Hz; 0 removes the filter.
};
AYLO_ECS_CALL_DATA(SetFilterArguments, "aylo.audio.SetFilterArguments", voice, lowpass);

/// @brief The arguments of `aylo.audio.stopVoice`.
struct StopVoiceArguments {
    std::uint32_t voice{0};  ///< From `aylo.audio.playSound`.
    float fadeOut{0.0F};     ///< Seconds to fade out over; 0 stops at once.
};
AYLO_ECS_CALL_DATA(StopVoiceArguments, "aylo.audio.StopVoiceArguments", voice, fadeOut);

/// @brief Registers the audio engine calls on @p app, for scripts: `aylo.audio.loadSound`, on the
/// `aylo.AudioAssets` resource, and `aylo.audio.playSound`, which sends a PlaySound and returns the voice's id (an
/// unknown bus throws an AudioCallException), `aylo.audio.getTime` (the audio clock), and `setVolume`, `setFilter` and
/// `stopVoice`, which send a ControlVoice for that id. Called by the AudioPlugin.
void addAudioCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addAudioForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::audio
