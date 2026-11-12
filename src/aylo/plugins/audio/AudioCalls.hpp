/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioCalls
*/

#pragma once

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
};
AYLO_ECS_CALL_DATA(PlaySoundArguments, "aylo.audio.PlaySoundArguments", sound, bus, volume, pitch, position);

/// @brief Registers the audio engine calls on @p app, for scripts: `aylo.audio.loadSound`, on the
/// `aylo.AudioAssets` resource, and `aylo.audio.playSound`, which sends a PlaySound (an unknown bus throws an
/// AudioLoadException). Called by the AudioPlugin.
void addAudioCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addAudioForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::audio
