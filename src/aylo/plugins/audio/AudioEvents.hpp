/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioEvents
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"      // IWYU pragma: keep
#include "plugins/audio/AudioReflection.hpp"  // IWYU pragma: keep
#include "plugins/audio/Spatial3D.hpp"

namespace aylo::plugins::audio {
/// @brief A one-shot sound, fire and forget (a shot, an explosion). Dropped if its sound is not loaded by the time the
/// audio plugin plays it: a late shot sound is worse than a missing one.
struct PlaySound {
    core::asset::AssetId sound;  ///< The sound to play, from AudioAssets::load().
    contract::audiodevice::Bus bus{contract::audiodevice::Bus::kEffects};  ///< The bus it is mixed into.
    float volume{1.0F};                                                    ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};                                                     ///< Playback speed, 1 = unchanged.
    std::optional<glm::vec2> position;  ///< World position to pan from; none: centred.
    /// Where it plays in 3D, panned and attenuated for the 3D listener, and again each frame while it plays; none: not
    /// a 3D sound (position, if any, pans it in 2D).
    std::optional<SoundPlace3D> place3D;
    bool loop{false};        ///< Plays until stopped.
    double startAt{0.0};     ///< When to start, in seconds of the audio clock; 0 or past: at once.
    float lowpass{0.0F};     ///< Low-pass cutoff in Hz; 0: none.
    std::uint32_t voice{0};  ///< The id `aylo.audio.playSound` gave the script; 0: not asked for.
};
AYLO_ECS_EVENT(PlaySound, "aylo.PlaySound");

/// @brief Changes or stops a voice a script started with `aylo.audio.playSound`, by the id the call gave. Does nothing
/// if that voice ended, was never played (its sound was not loaded) or was never given.
struct ControlVoice {
    std::uint32_t voice{0};        ///< The id `aylo.audio.playSound` gave.
    std::optional<float> volume;   ///< New linear gain.
    std::optional<float> lowpass;  ///< New low-pass cutoff in Hz; 0 removes it.
    bool stop{false};              ///< Stops the voice, after the changes above.
    float fadeOut{0.0F};           ///< Seconds the stop fades out over.
};
AYLO_ECS_EVENT(ControlVoice, "aylo.ControlVoice");

/// @brief Replaces the music: the current track fades out while this one fades in, looping on the music bus. Waits for
/// its sound if it is not loaded yet; a later PlayMusic or StopMusic replaces it.
struct PlayMusic {
    core::asset::AssetId sound;  ///< The track, best loaded with SoundDesc::stream.
    float crossfade{1.0F};       ///< Seconds.
};
AYLO_ECS_EVENT(PlayMusic, "aylo.PlayMusic", sound, crossfade);

/// @brief Fades the music out.
struct StopMusic {
    float fadeOut{1.0F};  ///< Seconds.
};
AYLO_ECS_EVENT(StopMusic, "aylo.StopMusic", fadeOut);
}  // namespace aylo::plugins::audio
