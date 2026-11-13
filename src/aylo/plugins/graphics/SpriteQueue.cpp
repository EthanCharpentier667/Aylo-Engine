/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SpriteQueue
*/

#include "plugins/graphics/SpriteQueue.hpp"

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"

namespace aylo::plugins::graphics {

contract::renderer::TextureId SpriteQueue::Item::getTexture() const {
    return std::visit([](const auto& drawn) { return drawn.texture; }, drawing);
}

contract::renderer::BlendMode SpriteQueue::Item::getBlend() const {
    using contract::renderer::BlendMode;
    if (const auto* sprite = std::get_if<contract::renderer::Sprite>(&drawing)) {
        return sprite->blend;
    }
    const BlendMode blend = std::visit([](const auto& drawn) { return drawn.blend; }, drawing);
    return blend == BlendMode::kCutout ? BlendMode::kAlpha : blend;  // only sprites have a cutout
}

void SpriteQueue::push(std::int32_t layer, const contract::renderer::Sprite& sprite, const std::uint64_t order) {
    _items.push_back(Item{.layer = layer, .order = order, .drawing = sprite});
}

void SpriteQueue::push(std::int32_t layer, contract::renderer::Text text, const std::uint64_t order) {
    _items.push_back(Item{.layer = layer, .order = order, .drawing = std::move(text)});
}

void SpriteQueue::push(std::int32_t layer, const contract::renderer::ParticleDraw& particles,
                       const std::uint64_t order) {
    _items.push_back(Item{.layer = layer, .order = order, .drawing = particles});
}

std::vector<SpriteQueue::Item> SpriteQueue::take() { return std::exchange(_items, {}); }

}  // namespace aylo::plugins::graphics
