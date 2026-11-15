/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Spatial3D
*/

#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include "plugins/audio/Export.hpp"

namespace aylo::plugins::audio {

/// @brief Where the 3D listener is and where it faces: the ears of a first-person camera. In metres, Y up, the
/// listener looking along -Z with +X on its right, as a Transform3D.
struct ListenerPose3D {
    glm::vec3 position{0.0F};                    ///< In metres.
    glm::vec4 rotation{0.0F, 0.0F, 0.0F, 1.0F};  ///< A quaternion x, y, z, w (normalized when read; zero: identity).
};

/// @brief Where a 3D sound plays from, and how its volume falls with the distance.
struct SoundPlace3D {
    glm::vec3 position{0.0F};  ///< In metres.
    float minDistance{1.0F};   ///< Closer than this, the sound plays at its full volume.
    float maxDistance{50.0F};  ///< Beyond this, it gets no quieter.
};

/// @brief What a listener hears of a 3D sound.
struct Spatial3D {
    float pan{0.0F};   ///< -1 left, 0 centre, 1 right.
    float gain{1.0F};  ///< What its volume is multiplied by, in (0, 1].
};

/// @brief How a listener hears a sound placed in 3D.
/// @details The pan is the sine of the sound's angle to the listener's front, in its horizontal plane: a sound on the
/// right is fully right, in front or behind it is centred. The gain is the inverse of the distance, clamped:
/// `minDistance / clamp(distance, minDistance, maxDistance)`, so a sound twice the minimum distance away plays at half
/// its volume, and stops fading at the maximum distance. A sound at the listener's place is centred, at full volume.
/// @param place Where the sound is; a non-positive minimum distance counts as 1 cm, a maximum below it as the minimum.
/// @param listener Where the listener is and faces.
[[nodiscard]] AYLO_PLUGIN_AUDIO_API Spatial3D spatialize(const SoundPlace3D& place,
                                                         const ListenerPose3D& listener) noexcept;

}  // namespace aylo::plugins::audio
