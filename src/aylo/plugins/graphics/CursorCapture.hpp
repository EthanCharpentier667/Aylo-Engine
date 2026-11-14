/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CursorCapture
*/

#pragma once

#include "aylo/ecs/resource/ResourceTraits.hpp"

namespace aylo::plugins::graphics {

/// @brief Whether the cursor is captured: hidden, locked to the window, the mouse deltas unbounded (mouse-look). An ECS
/// resource, "aylo.CursorCapture".
///
/// @details The game says what it wants (`window.captureCursor` in `project.luau`, then `aylo.graphics.captureCursor`);
/// the PreUpdate system "aylo.graphics.cursor" captures the cursor while the window has the focus and, when a tool (an
/// editor) draws over the game, while the tool lends it (ToolFrame::lendsCursor), and releases it otherwise, so a
/// player who switches window gets the cursor back.
struct CursorCapture {
    bool wanted{false};    ///< What the game asks for.
    bool focused{true};    ///< Whether the window has the focus, from its FocusChanged events.
    bool captured{false};  ///< Whether the cursor is captured now.
};
AYLO_ECS_RESOURCE(CursorCapture, "aylo.CursorCapture", wanted, focused, captured);

}  // namespace aylo::plugins::graphics
