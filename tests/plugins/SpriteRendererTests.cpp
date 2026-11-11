/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SpriteRendererTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <glm/ext/vector_float2.hpp>

#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/math/Rect.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"

static_assert(aylo::ecs::FieldTypeOf<aylo::core::asset::AssetId>::value == aylo::ecs::FieldType::kAssetId);
static_assert(aylo::ecs::Component<aylo::plugins::sprite::SpriteRenderer>);

TEST(SpriteRenderer, HasNeutralDefaults) {
    const aylo::plugins::sprite::SpriteRenderer sprite{};
    EXPECT_EQ(sprite.texture, aylo::core::asset::AssetId{});
    EXPECT_EQ(sprite.color, aylo::contract::renderer::Color{1.0F});
    EXPECT_EQ(sprite.origin, glm::vec2{0.0F});
    EXPECT_EQ(sprite.layer, 0);
    EXPECT_EQ(sprite.blend, aylo::contract::renderer::BlendMode::kAlpha);
    EXPECT_EQ(sprite.material, aylo::core::asset::AssetId{});
}

TEST(SpriteRenderer, ReflectsEveryFieldField) {
    using Traits = aylo::ecs::ComponentTraits<aylo::plugins::sprite::SpriteRenderer>;
    EXPECT_EQ(Traits::kName, "rtype.SpriteRenderer");
    ASSERT_EQ(Traits::kFields.size(), 7U);
    EXPECT_EQ(Traits::kFields[0].name, "texture");
    EXPECT_EQ(Traits::kFields[0].type, aylo::ecs::FieldType::kAssetId);
    EXPECT_EQ(Traits::kFields[1].name, "source");
    EXPECT_EQ(Traits::kFields[1].type, aylo::ecs::FieldType::kVec4);
    EXPECT_EQ(Traits::kFields[2].name, "color");
    EXPECT_EQ(Traits::kFields[2].type, aylo::ecs::FieldType::kVec4);
    EXPECT_EQ(Traits::kFields[3].name, "origin");
    EXPECT_EQ(Traits::kFields[3].type, aylo::ecs::FieldType::kVec2);
    EXPECT_EQ(Traits::kFields[4].name, "layer");
    EXPECT_EQ(Traits::kFields[4].type, aylo::ecs::FieldType::kI32);
    EXPECT_EQ(Traits::kFields[5].name, "blend");
    EXPECT_EQ(Traits::kFields[5].type, aylo::ecs::FieldType::kU8);
    EXPECT_EQ(Traits::kFields[6].name, "material");
    EXPECT_EQ(Traits::kFields[6].type, aylo::ecs::FieldType::kAssetId);
}

TEST(SpriteRenderer, ReflectsTheSourceAsItsPositionThenItsSize) {
    aylo::plugins::sprite::SpriteRenderer sprite{.source =
                                                     aylo::contract::renderer::math::Rect{{1.0F, 2.0F}, {3.0F, 4.0F}}};
    const auto& source = aylo::ecs::ComponentTraits<aylo::plugins::sprite::SpriteRenderer>::kFields[1];
    std::array<float, 4> read{};
    std::memcpy(read.data(), reinterpret_cast<const std::byte*>(&sprite) + source.offset, sizeof(read));  // NOLINT
    EXPECT_EQ(read, (std::array<float, 4>{1.0F, 2.0F, 3.0F, 4.0F}));
}

TEST(SpriteRenderer, IsStoredOnAnEntity) {
    aylo::ecs::World world;
    const aylo::ecs::Entity ship = world.spawn();
    world.getComponents().add(ship, aylo::plugins::sprite::SpriteRenderer{
                                        .texture = aylo::core::asset::AssetId::fromPath("ship.png"),
                                        .source = aylo::contract::renderer::math::Rect{{0.0F, 0.0F}, {32.0F, 16.0F}},
                                        .layer = 10});
    const auto* sprite = world.getComponents().get<aylo::plugins::sprite::SpriteRenderer>(ship);
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->texture, aylo::core::asset::AssetId::fromPath("ship.png"));
    EXPECT_FLOAT_EQ(sprite->source.getWidth(), 32.0F);
    EXPECT_EQ(sprite->layer, 10);
}
