/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioCalls
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <limits>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep

namespace aylo::plugins::audio {

inline constexpr std::string_view kLoadSoundCall = "aylo.audio.loadSound";          ///< AudioAssets::load().
inline constexpr std::string_view kSoundInfoCall = "aylo.audio.soundInfo";          ///< AudioAssets::probe().
inline constexpr std::string_view kPlaySoundCall = "aylo.audio.playSound";          ///< Sends a PlaySound.
inline constexpr std::string_view kPlaySound3DCall = "aylo.audio.playSound3D";      ///< A PlaySound placed in 3D.
inline constexpr std::string_view kSetListener3DCall = "aylo.audio.setListener3D";  ///< Audio::listener3D.
inline constexpr std::string_view kGetTimeCall = "aylo.audio.getTime";              ///< The audio clock.
inline constexpr std::string_view kGetOutputLatencyCall =
    "aylo.audio.getOutputLatency";                                          ///< IAudioDevice::getOutputLatency().
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

/// @brief The arguments of `aylo.audio.soundInfo`.
struct SoundInfoArguments {
    core::asset::AssetId sound;  ///< The sound, from `aylo.audio.loadSound` (or its path).
};
AYLO_ECS_CALL_DATA(SoundInfoArguments, "aylo.audio.SoundInfoArguments", sound);

/// @brief The result of `aylo.audio.soundInfo`: what the sound's file holds, or why it does not play.
struct SoundInfoResult {
    bool ok{false};               ///< The file is there and the engine decodes it; false: `error` says why.
    std::string error;            ///< Why not, when `ok` is false; empty otherwise.
    std::uint32_t sampleRate{0};  ///< The file's frames per second, before the device resamples it; 0 if not ok.
    std::uint32_t channels{0};    ///< The file's channels; 0 if not ok.
    std::uint64_t frames{0};      ///< The file's length, in frames (samples per channel); 0 if not ok.
    double seconds{0.0};          ///< frames / sampleRate; 0 if not ok.
};
AYLO_ECS_CALL_DATA(SoundInfoResult, "aylo.audio.SoundInfoResult", ok, error, sampleRate, channels, frames, seconds);

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

/// @brief The arguments of `aylo.audio.playSound3D`: a sound played from a place in 3D, panned and attenuated for the
/// 3D listener (`aylo.audio.setListener3D`, or the AudioListener entity with a Transform3D), every frame while it
/// plays.
struct PlaySound3DArguments {
    core::asset::AssetId sound;  ///< The sound, from `aylo.audio.loadSound` (or its path).
    ecs::String bus{"effects"};  ///< "effects" or "music".
    float volume{1.0F};          ///< Linear gain at the minimum distance, 1 = unchanged.
    float pitch{1.0F};           ///< Playback speed, 1 = unchanged.
    glm::vec3 position{0.0F};    ///< Where it plays from, in metres, Y up.
    float minDistance{1.0F};     ///< SoundPlace3D::minDistance, in metres, above 0.
    float maxDistance{50.0F};    ///< SoundPlace3D::maxDistance, in metres, at least minDistance.
    bool loop{false};            ///< Plays until `aylo.audio.stopVoice`.
    double startAt{0.0};         ///< Start time on the clock of `aylo.audio.getTime`, in seconds; 0 or past: at once.
    float lowpass{0.0F};         ///< Low-pass cutoff in Hz; 0: none.
};
AYLO_ECS_CALL_DATA(PlaySound3DArguments, "aylo.audio.PlaySound3DArguments", sound, bus, volume, pitch, position,
                   minDistance, maxDistance, loop, startAt, lowpass);

/// @brief The arguments of `aylo.audio.setListener3D`: the 3D listener's pose, until the next call.
struct SetListener3DArguments {
    glm::vec3 position{0.0F};                    ///< Where it hears from, in metres.
    glm::vec4 rotation{0.0F, 0.0F, 0.0F, 1.0F};  ///< Where it faces, a quaternion x, y, z, w (it looks along -Z).
};
AYLO_ECS_CALL_DATA(SetListener3DArguments, "aylo.audio.SetListener3DArguments", position, rotation);

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

/// @brief The result of `aylo.audio.getOutputLatency`.
struct LatencyResult {
    double latency{0.0};  ///< Seconds from the audio clock to the speaker; 0 without a device (a server) or output.
};
AYLO_ECS_CALL_DATA(LatencyResult, "aylo.audio.LatencyResult", latency);

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
/// `aylo.AudioAssets` resource, `aylo.audio.soundInfo` (what its file holds, AudioAssets::probe()), and
/// `aylo.audio.playSound`, which sends a PlaySound and returns the voice's id (an
/// unknown bus throws an AudioCallException), `aylo.audio.getTime` (the audio clock),
/// `aylo.audio.getOutputLatency` (from that clock to the speaker), and `setVolume`, `setFilter` and
/// `stopVoice`, which send a ControlVoice for that id. Called by the AudioPlugin.
void addAudioCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addAudioForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::audio
