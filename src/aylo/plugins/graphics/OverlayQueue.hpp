/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** OverlayQueue
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/drawing/Overlay.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"

namespace aylo::plugins::graphics {

/// @brief What the frame draws over everything else, in view pixels, as the ECS resource "aylo.OverlayQueue": the
/// GraphicsPlugin hands it to IRenderer::drawOverlay() just before it ends the frame.
///
/// @details Render systems push to it after "aylo.graphics.camera" and before "aylo.graphics.endFrame". They take it
/// as a ResAppend and never touch the renderer, so they need no order with the other plugins that draw. Items stay in
/// the order pushed.
class OverlayQueue {
  public:
    OverlayQueue() = default;
    ~OverlayQueue() = default;
    OverlayQueue(const OverlayQueue& other) = default;
    OverlayQueue& operator=(const OverlayQueue& other) = default;
    OverlayQueue(OverlayQueue&& other) noexcept = default;
    OverlayQueue& operator=(OverlayQueue&& other) noexcept = default;

    /// @brief Adds @p item on top of what was pushed.
    void push(contract::renderer::OverlayItem item) { _items.push_back(std::move(item)); }

    /// @return The items pushed since the last call, in order, leaving the queue empty.
    [[nodiscard]] std::vector<contract::renderer::OverlayItem> take() { return std::exchange(_items, {}); }

  private:
    std::vector<contract::renderer::OverlayItem> _items;  ///< In draw order.
};
AYLO_ECS_RESOURCE(OverlayQueue, "aylo.OverlayQueue");

/// @brief The sizes of the frame, as the ECS resource "aylo.ViewInfo", written each frame before the Render systems
/// that draw: what a plugin needs to place things on screen without touching the backend.
struct ViewInfo {
    glm::vec2 viewSize{0.0F};   ///< The view: what overlay items are placed in, in view pixels.
    glm::uvec2 framebuffer{0};  ///< The framebuffer, in pixels; 0x0 while minimized.
    glm::uvec2 window{0};       ///< The window, in the units of the mouse events.
};
AYLO_ECS_RESOURCE(ViewInfo, "aylo.ViewInfo");

}  // namespace aylo::plugins::graphics
