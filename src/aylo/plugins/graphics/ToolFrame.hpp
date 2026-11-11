/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ToolFrame
*/

#pragma once

#include <functional>

#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"

namespace aylo::plugins::graphics {

/// @brief The frame of a tool (an editor, a debug UI) that draws over the game, as the main-thread ECS resource
/// "aylo.ToolFrame".
///
/// @details The GraphicsPlugin's Render system "aylo.graphics.endFrame" calls it once per frame, after the overlay and
/// just before it ends the frame, with the platform and the renderer it owns: the tool draws meshes
/// (IRenderer::drawMesh()), asks for the scene to go to a texture (IRenderer::setSceneSize()) and reads what the frame
/// drew. It runs where the graphics plugin already owns the backend, so a tool needs no order against the plugins that
/// draw. It is not called while the window is minimized. Nothing to run, the default, costs nothing.
struct ToolFrame {
    std::function<void(contract::platform::IPlatform&, contract::renderer::IRenderer&)> draw;  ///< The tool's frame.
};
AYLO_ECS_RESOURCE(ToolFrame, "aylo.ToolFrame");

}  // namespace aylo::plugins::graphics
