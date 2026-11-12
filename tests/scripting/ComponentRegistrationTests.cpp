/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentRegistrationTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <string>

#include "aylo/ecs/world/World.hpp"
#include "aylo/scripting/component/ComponentDeclaration.hpp"
#include "aylo/scripting/component/ComponentRegistration.hpp"

namespace {
namespace sc = aylo::scripting::component;
using aylo::ecs::FieldType;
using aylo::scripting::ErrorKind;

sc::ComponentDeclaration makeBullet() {
    return {.name = "game.Bullet",
            .fields = {{.name = "damage", .type = FieldType::kF32}, {.name = "owner", .type = FieldType::kEntity}}};
}

sc::ComponentDeclaration makeShield() {
    return {.name = "game.Shield",
            .fields = {{.name = "strength", .type = FieldType::kF32}, {.name = "regen", .type = FieldType::kF32}}};
}
}  // namespace

TEST(ComponentRegistration, SortsByAlignmentFirst) {
    aylo::ecs::World world;
    const auto layout = sc::registerComponent(world.getComponents(), makeBullet());
    ASSERT_TRUE(layout);
    EXPECT_EQ(layout->size, 16U);
    EXPECT_EQ(layout->alignment, 8U);
    ASSERT_EQ(layout->fields.size(), 2U);
    EXPECT_EQ(layout->fields[0].name, "owner");
    EXPECT_EQ(layout->fields[0].offset, 0U);
    EXPECT_EQ(layout->fields[1].name, "damage");
    EXPECT_EQ(layout->fields[1].offset, 8U);
}

TEST(ComponentRegistration, SortsByNameWithinTheSameAlignment) {
    aylo::ecs::World world;
    const auto layout = sc::registerComponent(world.getComponents(), makeShield());
    ASSERT_TRUE(layout);
    EXPECT_EQ(layout->size, 8U);
    EXPECT_EQ(layout->alignment, 4U);
    EXPECT_EQ(layout->fields[0].name, "regen");
    EXPECT_EQ(layout->fields[1].name, "strength");
}

TEST(ComponentRegistration, RegistersTheTypeInTheWorld) {
    aylo::ecs::World world;
    const auto layout = sc::registerComponent(world.getComponents(), makeShield());
    ASSERT_TRUE(layout);
    EXPECT_EQ(world.getComponents().find("game.Shield"), layout->id);
}

TEST(ComponentRegistration, SameDeclarationTwiceReturnsTheSameId) {
    aylo::ecs::World world;
    const auto first = sc::registerComponent(world.getComponents(), makeShield());
    const auto second = sc::registerComponent(world.getComponents(), makeShield());
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->id, second->id);
}

TEST(ComponentRegistration, FieldOrderInTheDeclarationDoesNotChangeTheLayout) {
    aylo::ecs::World world;
    sc::ComponentDeclaration reversed = makeShield();
    std::swap(reversed.fields[0], reversed.fields[1]);
    const auto first = sc::registerComponent(world.getComponents(), makeShield());
    const auto second = sc::registerComponent(world.getComponents(), reversed);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->id, second->id);
}

TEST(ComponentRegistration, AnotherLayoutUnderTheSameNameIsRefused) {
    aylo::ecs::World world;
    ASSERT_TRUE(sc::registerComponent(world.getComponents(), makeShield()));
    sc::ComponentDeclaration other = makeShield();
    other.fields[0].type = FieldType::kF64;
    const auto result = sc::registerComponent(world.getComponents(), other);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kLayoutMismatch);
}

TEST(ComponentRegistration, DuplicateFieldIsRefused) {
    aylo::ecs::World world;
    sc::ComponentDeclaration declaration = makeShield();
    declaration.fields[1].name = "strength";
    const auto result = sc::registerComponent(world.getComponents(), declaration);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kInvalidDeclaration);
    EXPECT_NE(std::string{result.error().message()}.find("strength"), std::string::npos);
}

TEST(ComponentRegistration, EmptyNamesAreRefused) {
    aylo::ecs::World world;
    sc::ComponentDeclaration noName = makeShield();
    noName.name.clear();
    EXPECT_EQ(sc::registerComponent(world.getComponents(), noName).error().kind(), ErrorKind::kInvalidDeclaration);
    sc::ComponentDeclaration noFieldName = makeShield();
    noFieldName.fields[0].name.clear();
    EXPECT_EQ(sc::registerComponent(world.getComponents(), noFieldName).error().kind(), ErrorKind::kInvalidDeclaration);
}

TEST(ComponentRegistration, DefaultsAreWrittenAtTheFieldOffsets) {
    aylo::ecs::World world;
    sc::ComponentDeclaration declaration = makeBullet();
    const float damage = 10.0F;
    declaration.fields[0].defaultValue.resize(sizeof(float));
    std::memcpy(declaration.fields[0].defaultValue.data(), &damage, sizeof(float));
    const auto layout = sc::registerComponent(world.getComponents(), declaration);
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->defaults.size(), 16U);
    float stored = 0.0F;
    std::memcpy(&stored, layout->defaults.data() + 8, sizeof(float));
    EXPECT_EQ(stored, damage);
    EXPECT_EQ(layout->defaults[0], std::byte{0});
}

TEST(ComponentRegistration, DefaultOfTheWrongSizeIsRefused) {
    aylo::ecs::World world;
    sc::ComponentDeclaration declaration = makeShield();
    declaration.fields[0].defaultValue.resize(3);
    const auto result = sc::registerComponent(world.getComponents(), declaration);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kInvalidDeclaration);
}
