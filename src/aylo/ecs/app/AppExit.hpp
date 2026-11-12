/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AppExit
*/

#pragma once

#include "aylo/ecs/event/EventTraits.hpp"

namespace aylo::ecs {
/// @brief Asks the App to stop: App::run() returns `code` after the frame the event was sent in.
/// @details Any system may send it (the window plugin when the window closes, the server on a signal); when several
/// are sent, the first one read gives the code.
/// @code
/// void quit(aylo::ecs::EventWriter<aylo::ecs::AppExit> exit) {
///     exit.send(aylo::ecs::AppExit{.code = 0});
/// }
/// @endcode
struct AppExit {
    int code;  ///< What App::run() returns: 0 for success.
};
AYLO_ECS_EVENT(AppExit, "aylo.AppExit", code);
}  // namespace aylo::ecs
