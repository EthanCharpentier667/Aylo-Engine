/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WindowCalls
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallTraits.hpp"

namespace aylo::plugins::graphics {

inline constexpr std::string_view kCaptureCursorCall = "aylo.graphics.captureCursor";  ///< CursorCapture::wanted.

/// @brief The arguments of `aylo.graphics.captureCursor`.
struct CaptureCursorArguments {
    bool capture{true};  ///< true captures the cursor (mouse-look), false gives it back.
};
AYLO_ECS_CALL_DATA(CaptureCursorArguments, "aylo.graphics.CaptureCursorArguments", capture);

/// @brief Registers the window engine call on @p app, for scripts: `aylo.graphics.captureCursor`, on the
/// `aylo.CursorCapture` resource. It takes effect at the next PreUpdate ("aylo.graphics.cursor"). Called by the
/// GraphicsPlugin.
void addWindowCalls(ecs::App& app);

}  // namespace aylo::plugins::graphics
