/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TextRenderer
*/

#pragma once

#include <array>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <string_view>
#include <type_traits>

#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "plugins/text/TextLayout.hpp"

/// @brief A text::TextAlign field is reflected as kU8, as a BlendMode is (see SpriteRenderer.hpp).
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::text::TextAlign>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a TextAlign field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::text::TextAlign> {
    static constexpr std::array<std::string_view, 3> kNames{"left", "center", "right"};
};
static_assert(sizeof(aylo::plugins::text::TextAlign) == 1, "TextAlign must match FieldType::kU8");

namespace aylo::plugins::text {
/// @brief Draws a text on an entity: the component the TextPlugin lays out (layoutText()) and draws with the sprites.
///
/// @details Only what to draw: where (position, scale, rotation) comes from the entity's GlobalTransform2D, the anchor
/// saying which point of the text sits there. The font is referred to by its AssetId, which FontAssets::load() returns
/// at once; the text is drawn once its font and its atlas are loaded, sorted with the sprites by layer. Its fields are
/// reflected (Luau, network).
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform2D{.position = {400.0F, 300.0F}})
///     .add(aylo::plugins::text::TextRenderer{.text = "Player 1 wins", .font = orbitron, .size = 48.0F,
///                                             .anchor = {0.5F, 0.5F}});
/// @endcode
struct TextRenderer {
    std::string text;                       ///< UTF-8; `\n` starts a new line.
    core::asset::AssetId font;              ///< From FontAssets::load().
    float size = 32.0F;                     ///< The font size: world units per em, before the entity's scale.
    contract::renderer::Color color{1.0F};  ///< Fills the glyphs: colour and transparency.
    TextAlign align = TextAlign::kLeft;     ///< How the lines line up, for a text of several lines.
    glm::vec2 anchor{0.0F};  ///< The point of the text on the entity's position: (0, 0) its top-left corner, (0.5, 0.5)
                             ///< its centre, (1, 1) its bottom-right corner. It is also the pivot of the rotation.
    std::int32_t layer{0};   ///< Draw order among the sprites: higher layers are drawn on top.
    contract::renderer::BlendMode blend = contract::renderer::BlendMode::kAlpha;  ///< kAlpha or kAdditive.
    core::asset::AssetId material;  ///< From MaterialAssets; the default material if empty or not created yet. Its
                                    ///< `instance uniform` values come from the entity's MaterialInstance.
};
AYLO_ECS_COMPONENT(TextRenderer, "aylo.TextRenderer", text, font, size, color, align, anchor, layer, material);
}  // namespace aylo::plugins::text
