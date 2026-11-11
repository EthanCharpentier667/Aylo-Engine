/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioSource
*/

#pragma once

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"      // IWYU pragma: keep
#include "plugins/audio/AudioReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::audio {
/// @brief A sound attached to an entity: starts once its sound is loaded, stops when the component or the entity goes.
/// @details The audio plugin follows it every frame: a new sound restarts the voice, volume and pitch apply at once,
/// and a spatial source is panned from its GlobalTransform relative to the AudioListener. The bus and the loop are
/// read when the voice starts. A one-shot source plays once, and does not restart while the component stays.
/// @code
/// const aylo::core::asset::AssetId engine = sounds->load("assets/sounds/engine.ogg");
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform{})
///     .add(aylo::plugins::audio::AudioSource{.sound = engine, .loop = true});
/// @endcode
struct AudioSource {
    core::asset::AssetId sound;  ///< The sound to play, from AudioAssets::load().
    contract::audiodevice::Bus bus{contract::audiodevice::Bus::kEffects};  ///< The bus it is mixed into.
    float volume{1.0F};                                                    ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};                                                     ///< Playback speed, 1 = unchanged.
    bool loop{false};                                                      ///< Plays until removed, instead of once.
    bool spatial{true};  ///< Panned from its GlobalTransform; false, or no GlobalTransform: centred.
};
AYLO_ECS_COMPONENT(AudioSource, "rtype.AudioSource", sound, bus, volume, pitch, loop, spatial);
}  // namespace aylo::plugins::audio
