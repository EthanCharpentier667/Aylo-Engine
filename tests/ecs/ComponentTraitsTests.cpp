/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentTraitsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"

namespace aylo::test::traits {
/// @brief A math type of the game, reflected through a FieldTypeOf specialization.
struct Vec2 {
    float x;
    float y;
};
}  // namespace aylo::test::traits

template <>
struct aylo::ecs::FieldTypeOf<aylo::test::traits::Vec2>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec2> {};

namespace aylo::test::traits {
struct Health {
    std::int32_t value;
    std::int32_t max;
};
AYLO_ECS_COMPONENT(Health, "rtype.Health", value, max);

struct Path {
    std::array<Vec2, 4> points;
    std::array<float, 3> speeds;
    std::uint8_t flags;
};
AYLO_ECS_COMPONENT(Path, "test.Path", points, speeds, flags);

struct Dead {};
AYLO_ECS_COMPONENT(Dead, "rtype.Dead");

struct Scaled {
    float scale{1.0F};
};
AYLO_ECS_COMPONENT(Scaled, "test.Scaled", scale);

struct Link {
    aylo::ecs::Entity target;
    std::array<aylo::ecs::Entity, 2> pair;
};
AYLO_ECS_COMPONENT(Link, "test.Link", target, pair);

struct Inventory {
    std::vector<std::int32_t> items;
    std::int32_t gold{5};
};
AYLO_ECS_COMPONENT(Inventory, "test.Inventory");
}  // namespace aylo::test::traits

struct GlobalPosition {
    float x;
    float y;
};
AYLO_ECS_COMPONENT(GlobalPosition, "test.GlobalPosition", x, y);

static_assert(aylo::ecs::Component<aylo::test::traits::Health>);
static_assert(aylo::ecs::Component<GlobalPosition>);
static_assert(!aylo::ecs::Component<int>);
static_assert(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kName == "rtype.Health");
static_assert(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kLayoutHash != 0, "known at compile time");

namespace {
/// @brief Raw storage for one T, as a pool would hold it.
template <typename T>
struct Storage {
    alignas(T) std::array<std::byte, sizeof(T)> bytes;

    void* raw() { return bytes.data(); }
    T& get() { return *std::launder(reinterpret_cast<T*>(bytes.data())); }
};
}  // namespace

TEST(ComponentTraits, RecordsNameAndHash) {
    using Traits = aylo::ecs::ComponentTraits<aylo::test::traits::Health>;
    EXPECT_EQ(Traits::kName, "rtype.Health");
    EXPECT_EQ(Traits::kHash, aylo::core::hash::hashName("rtype.Health"));
}

TEST(ComponentTraits, WorksInsideANamespaceAndAtGlobalScope) {
    EXPECT_EQ(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kFields.size(), 2U);
    EXPECT_EQ(aylo::ecs::ComponentTraits<GlobalPosition>::kName, "test.GlobalPosition");
    EXPECT_EQ(aylo::ecs::ComponentTraits<GlobalPosition>::kFields.at(1).name, "y");
}

TEST(ComponentTraits, FieldsMatchTheStruct) {
    const auto& fields = aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kFields;
    EXPECT_EQ(fields.at(0).name, "value");
    EXPECT_EQ(fields.at(0).type, aylo::ecs::FieldType::kI32);
    EXPECT_EQ(fields.at(0).offset, offsetof(aylo::test::traits::Health, value));
    EXPECT_EQ(fields.at(1).name, "max");
    EXPECT_EQ(fields.at(1).offset, offsetof(aylo::test::traits::Health, max));
    EXPECT_EQ(fields.at(1).count, 1U);
}

TEST(ComponentTraits, ReflectsArraysAndMappedTypes) {
    const auto& fields = aylo::ecs::ComponentTraits<aylo::test::traits::Path>::kFields;
    EXPECT_EQ(fields.at(0).name, "points");
    EXPECT_EQ(fields.at(0).type, aylo::ecs::FieldType::kVec2);
    EXPECT_EQ(fields.at(0).count, 4U);
    EXPECT_EQ(fields.at(1).type, aylo::ecs::FieldType::kF32);
    EXPECT_EQ(fields.at(1).count, 3U);
    EXPECT_EQ(fields.at(1).offset, offsetof(aylo::test::traits::Path, speeds));
    EXPECT_EQ(fields.at(2).type, aylo::ecs::FieldType::kU8);
    EXPECT_EQ(fields.at(2).offset, offsetof(aylo::test::traits::Path, flags));
}

TEST(ComponentTraits, AnEmptyStructIsATag) {
    const aylo::ecs::ComponentInfo info = aylo::ecs::ComponentTraits<aylo::test::traits::Dead>::makeInfo();
    EXPECT_EQ(info.type.size, 0U);
    EXPECT_TRUE(info.type.fields.empty());
}

TEST(ComponentTraits, MakeInfoDescribesTheComponent) {
    const aylo::ecs::ComponentInfo info = aylo::ecs::ComponentTraits<aylo::test::traits::Path>::makeInfo();
    EXPECT_EQ(info.type.name, "test.Path");
    EXPECT_EQ(info.type.size, sizeof(aylo::test::traits::Path));
    EXPECT_EQ(info.type.alignment, alignof(aylo::test::traits::Path));
    ASSERT_EQ(info.type.fields.size(), 3U);
    EXPECT_EQ(info.type.fields.at(0).name, "points");
    EXPECT_EQ(info.type.fields.at(0).count, 4U);
    EXPECT_EQ(info.type.id, aylo::ecs::kInvalidComponentId);  // left to the registry
}

TEST(ComponentTraits, EveryTestComponentIsValid) {
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::makeInfo()));
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<aylo::test::traits::Path>::makeInfo()));
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<aylo::test::traits::Dead>::makeInfo()));
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<aylo::test::traits::Scaled>::makeInfo()));
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<aylo::test::traits::Inventory>::makeInfo()));
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<GlobalPosition>::makeInfo()));
}

TEST(TypeHooks, AreNullForPlainData) {
    const aylo::ecs::TypeHooks hooks = aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kHooks;
    EXPECT_EQ(hooks.construct, nullptr);
    EXPECT_EQ(hooks.destruct, nullptr);
    EXPECT_EQ(hooks.move, nullptr);
    EXPECT_EQ(hooks.copy, nullptr);
}

TEST(TypeHooks, ConstructAppliesDefaultMemberInitializers) {
    const aylo::ecs::TypeHooks hooks = aylo::ecs::ComponentTraits<aylo::test::traits::Scaled>::kHooks;
    ASSERT_NE(hooks.construct, nullptr);
    EXPECT_EQ(hooks.destruct, nullptr);
    EXPECT_EQ(hooks.move, nullptr);
    EXPECT_EQ(hooks.copy, nullptr);

    Storage<aylo::test::traits::Scaled> storage{};
    hooks.construct(storage.raw());
    EXPECT_EQ(storage.get().scale, 1.0F);
}

TEST(TypeHooks, ManageNonTrivialMembers) {
    const aylo::ecs::TypeHooks hooks = aylo::ecs::ComponentTraits<aylo::test::traits::Inventory>::kHooks;
    ASSERT_NE(hooks.construct, nullptr);
    ASSERT_NE(hooks.destruct, nullptr);
    ASSERT_NE(hooks.move, nullptr);
    ASSERT_NE(hooks.copy, nullptr);

    Storage<aylo::test::traits::Inventory> original{};
    hooks.construct(original.raw());
    EXPECT_EQ(original.get().gold, 5);
    original.get().items = {1, 2, 3};

    Storage<aylo::test::traits::Inventory> copy{};
    hooks.copy(copy.raw(), original.raw());
    EXPECT_EQ(copy.get().items, (std::vector<std::int32_t>{1, 2, 3}));

    Storage<aylo::test::traits::Inventory> moved{};
    hooks.move(moved.raw(), copy.raw());
    EXPECT_EQ(moved.get().items, (std::vector<std::int32_t>{1, 2, 3}));
    EXPECT_EQ(original.get().items.size(), 3U);

    hooks.destruct(original.raw());
    hooks.destruct(copy.raw());
    hooks.destruct(moved.raw());
}

TEST(ComponentTraits, LayoutHashMatchesTheRunTimeHash) {
    EXPECT_EQ(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kLayoutHash,
              aylo::ecs::computeLayoutHash(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::makeInfo().type));
    EXPECT_EQ(aylo::ecs::ComponentTraits<aylo::test::traits::Path>::kLayoutHash,
              aylo::ecs::computeLayoutHash(aylo::ecs::ComponentTraits<aylo::test::traits::Path>::makeInfo().type));
    EXPECT_EQ(aylo::ecs::ComponentTraits<aylo::test::traits::Dead>::kLayoutHash,
              aylo::ecs::computeLayoutHash(aylo::ecs::ComponentTraits<aylo::test::traits::Dead>::makeInfo().type));
    EXPECT_EQ(aylo::ecs::ComponentTraits<aylo::test::traits::Inventory>::kLayoutHash,
              aylo::ecs::computeLayoutHash(aylo::ecs::ComponentTraits<aylo::test::traits::Inventory>::makeInfo().type));
    EXPECT_EQ(aylo::ecs::ComponentTraits<GlobalPosition>::kLayoutHash,
              aylo::ecs::computeLayoutHash(aylo::ecs::ComponentTraits<GlobalPosition>::makeInfo().type));
}

TEST(ComponentTraits, LayoutHashDiffersBetweenComponents) {
    EXPECT_NE(aylo::ecs::ComponentTraits<aylo::test::traits::Health>::kLayoutHash,
              aylo::ecs::ComponentTraits<aylo::test::traits::Path>::kLayoutHash);
    EXPECT_NE(aylo::ecs::ComponentTraits<aylo::test::traits::Scaled>::kLayoutHash,
              aylo::ecs::ComponentTraits<aylo::test::traits::Dead>::kLayoutHash);
}

TEST(ComponentTraits, ReflectsEntityFields) {
    const auto& fields = aylo::ecs::ComponentTraits<aylo::test::traits::Link>::kFields;
    ASSERT_EQ(fields.size(), 2U);
    EXPECT_EQ(fields[0].type, aylo::ecs::FieldType::kEntity);
    EXPECT_EQ(fields[0].count, 1U);
    EXPECT_EQ(fields[0].offset, offsetof(aylo::test::traits::Link, target));
    EXPECT_EQ(fields[1].type, aylo::ecs::FieldType::kEntity);
    EXPECT_EQ(fields[1].count, 2U);
    EXPECT_EQ(fields[1].offset, offsetof(aylo::test::traits::Link, pair));
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::ComponentTraits<aylo::test::traits::Link>::makeInfo()));
}
