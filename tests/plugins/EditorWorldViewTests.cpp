/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorWorldViewTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/world/World.hpp"
#include "editor/gui/GuiMesh.hpp"
#include "editor/panels/WorldView.hpp"

namespace {
struct Health {
    std::int32_t value{0};
};
AYLO_ECS_COMPONENT(Health, "game.Health", value);

struct Armor {
    std::int32_t value{0};
};
AYLO_ECS_COMPONENT(Armor, "game.Armor", value);

struct Poison {
    std::int32_t value{0};
};
AYLO_ECS_COMPONENT(Poison, "other.Poison", value);

struct Burning {
    std::int32_t value{0};
};
AYLO_ECS_COMPONENT(Burning, "Burning", value);
}  // namespace

TEST(EditorWorldView, ShortensANameToWhatFollowsItsNamespace) {
    EXPECT_EQ(aylo::editor::panels::shortName("aylo.Transform"), "Transform");
    EXPECT_EQ(aylo::editor::panels::shortName("a.b.Deep"), "Deep");
    EXPECT_EQ(aylo::editor::panels::shortName("Plain"), "Plain");
    EXPECT_EQ(aylo::editor::panels::shortName(""), "");
}

TEST(EditorWorldView, ListsNothingForAnEmptyWorld) {
    aylo::ecs::World world;
    EXPECT_TRUE(aylo::editor::panels::collectEntities(world, "").empty());
}

TEST(EditorWorldView, ListsEachEntityOnceByIndex) {
    aylo::ecs::World world;
    const aylo::ecs::Entity first = world.spawn(Health{.value = 1}, Armor{.value = 2});
    const aylo::ecs::Entity second = world.spawn(Armor{.value = 3});
    const aylo::ecs::Entity third = world.spawn(Poison{.value = 4});
    const std::vector<aylo::ecs::Entity> entities = aylo::editor::panels::collectEntities(world, "");
    ASSERT_EQ(entities.size(), 3U);
    EXPECT_EQ(entities[0], first);
    EXPECT_EQ(entities[1], second);
    EXPECT_EQ(entities[2], third);
}

TEST(EditorWorldView, DoesNotListAnEntityWithoutAComponent) {
    aylo::ecs::World world;
    static_cast<void>(world.spawn());
    const aylo::ecs::Entity listed = world.spawn(Health{});
    const std::vector<aylo::ecs::Entity> entities = aylo::editor::panels::collectEntities(world, "");
    ASSERT_EQ(entities.size(), 1U);
    EXPECT_EQ(entities[0], listed);
}

TEST(EditorWorldView, KeepsTheEntitiesWithAComponentWhoseNameHoldsTheFilter) {
    aylo::ecs::World world;
    const aylo::ecs::Entity health = world.spawn(Health{});
    const aylo::ecs::Entity both = world.spawn(Armor{}, Poison{});
    static_cast<void>(world.spawn(Burning{}));
    const std::vector<aylo::ecs::Entity> entities = aylo::editor::panels::collectEntities(world, "game.");
    ASSERT_EQ(entities.size(), 2U);
    EXPECT_EQ(entities[0], health);
    EXPECT_EQ(entities[1], both);
}

TEST(EditorWorldView, IgnoresTheCaseOfTheFilter) {
    aylo::ecs::World world;
    const aylo::ecs::Entity entity = world.spawn(Health{});
    const std::vector<aylo::ecs::Entity> entities = aylo::editor::panels::collectEntities(world, "hEaLtH");
    ASSERT_EQ(entities.size(), 1U);
    EXPECT_EQ(entities[0], entity);
}

TEST(EditorWorldView, ListsNothingWhenNoComponentMatchesTheFilter) {
    aylo::ecs::World world;
    static_cast<void>(world.spawn(Health{}));
    EXPECT_TRUE(aylo::editor::panels::collectEntities(world, "nothing").empty());
}

TEST(EditorWorldView, DescribesAnEntityByItsIndexAndItsComponents) {
    aylo::ecs::World world;
    const aylo::ecs::Entity entity = world.spawn(Health{}, Armor{});
    const std::string line = aylo::editor::panels::describeEntity(world, entity);
    EXPECT_EQ(line, std::format("#{}  Health, Armor", entity.getIndex()));
}

TEST(EditorWorldView, DescribesOnlyTheFirstComponentsOfABigEntity) {
    aylo::ecs::World world;
    const aylo::ecs::Entity entity = world.spawn(Health{}, Armor{}, Poison{}, Burning{});
    const std::string line = aylo::editor::panels::describeEntity(world, entity);
    EXPECT_EQ(line, std::format("#{}  Health, Armor, Poison, ...", entity.getIndex()));
}

TEST(EditorGuiMesh, KeepsATextureIdThroughImGui) {
    const aylo::contract::renderer::TextureId texture{.index = 7, .generation = 3};
    const std::uint64_t packed = aylo::editor::gui::toImGuiTexture(texture);
    EXPECT_EQ(aylo::editor::gui::fromImGuiTexture(packed), texture);
}

TEST(EditorGuiMesh, KeepsTheLargestTextureIdThroughImGui) {
    const aylo::contract::renderer::TextureId texture{.index = 0xFFFFFFFFU, .generation = 0xFFFFFFFFU};
    EXPECT_EQ(aylo::editor::gui::fromImGuiTexture(aylo::editor::gui::toImGuiTexture(texture)), texture);
}

TEST(EditorGuiMesh, GivesNoTextureAnIdImGuiReadsAsNone) {
    EXPECT_EQ(aylo::editor::gui::toImGuiTexture(aylo::contract::renderer::TextureId{}), 0U);
    EXPECT_EQ(aylo::editor::gui::fromImGuiTexture(0), aylo::contract::renderer::TextureId{});
}
