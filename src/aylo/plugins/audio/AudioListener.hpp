/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioListener
*/

#pragma once

#include "aylo/ecs/component/ComponentTraits.hpp"

namespace aylo::plugins::audio {
/// @brief Where the player hears from: sounds are panned relative to the GlobalTransform of the entity with this
/// component (usually the camera). Without one, nothing is panned; with several, the first one found wins.
struct AudioListener {};
AYLO_ECS_COMPONENT(AudioListener, "rtype.AudioListener");
}  // namespace aylo::plugins::audio
