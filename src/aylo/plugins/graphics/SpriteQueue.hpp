/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SpriteQueue
*/

#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "plugins/graphics/Export.hpp"

namespace aylo::plugins::graphics {

/// @brief The sprites, texts and particles to draw this frame (the sprite entities, the tiles of a tilemap, the texts
/// of the text plugin, the emitters of the particle plugin), as the ECS resource "aylo.SpriteQueue".
///
/// @details Render systems push what they draw, after the GraphicsPlugin's "aylo.graphics.camera" and before its
/// "aylo.graphics.sprites", which draws it and empties the queue. There is no depth buffer: the draw order is the
/// order things cover each other. Within a frame it is, from first to last:
/// - by layer, lowest first;
/// - in a layer, the cutout items, then the alpha-blended ones, then the additive ones;
/// - among cutout or alpha items, by their `order` (the entity's index), then the order they were pushed: two items
///   that overlap keep the same order every frame, however the ECS iterates;
/// - among additive items, by texture: adding is the same in any order, so they are grouped into fewer draws.
/// They take it as a ResAppend, so they need no order between them.
/// @code
/// void drawStars(aylo::ecs::ResAppend<aylo::plugins::graphics::SpriteQueue> queue) {
///     queue->push(-10, aylo::contract::renderer::Sprite{.texture = star, .position = {100.0F, 50.0F}});
/// }
/// @endcode
class AYLO_PLUGIN_GRAPHICS_API SpriteQueue {
  public:
    /// @brief One sprite, text or emitter's particles, with the layer it is sorted by.
    struct AYLO_PLUGIN_GRAPHICS_API Item {
        std::int32_t layer{0};   ///< Draw order: higher on top, as SpriteRenderer::layer.
        std::uint64_t order{0};  ///< Order among the cutout or alpha items of its layer: the entity's index.
        std::variant<contract::renderer::Sprite, contract::renderer::Text, contract::renderer::ParticleDraw>
            drawing;  ///< The draw command; its texture must be loaded.

        /// @return The texture @p drawing is drawn with, the sort key of the additive items.
        [[nodiscard]] contract::renderer::TextureId getTexture() const;

        /// @return How @p drawing is blended; a text or particles in kCutout are drawn as kAlpha, so they say kAlpha.
        [[nodiscard]] contract::renderer::BlendMode getBlend() const;
    };

    SpriteQueue() = default;
    ~SpriteQueue() = default;
    SpriteQueue(const SpriteQueue& other) = default;
    SpriteQueue& operator=(const SpriteQueue& other) = default;
    SpriteQueue(SpriteQueue&& other) noexcept = default;
    SpriteQueue& operator=(SpriteQueue&& other) noexcept = default;

    /// @return The items pushed since the last take(), in order.
    [[nodiscard]] const std::vector<Item>& getItems() const noexcept { return _items; }

    /// @brief Adds @p sprite to the frame, sorted by @p layer then @p order (see the class).
    void push(std::int32_t layer, const contract::renderer::Sprite& sprite, std::uint64_t order = 0);

    /// @brief Adds @p text to the frame, sorted by @p layer then @p order.
    void push(std::int32_t layer, contract::renderer::Text text, std::uint64_t order = 0);

    /// @brief Adds the particles of one emitter to the frame, sorted by @p layer then @p order. CPU instances are not
    /// copied: they must stay valid until the GraphicsPlugin's "aylo.graphics.sprites" has drawn the queue.
    void push(std::int32_t layer, const contract::renderer::ParticleDraw& particles, std::uint64_t order = 0);

    /// @return The items pushed since the last call, in order, leaving the queue empty.
    [[nodiscard]] std::vector<Item> take();

  private:
    std::vector<Item> _items;  ///< In the order they were pushed.
};
AYLO_ECS_RESOURCE(SpriteQueue, "aylo.SpriteQueue");

}  // namespace aylo::plugins::graphics
