/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TypeTraitsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::test::type_traits {
struct Health {
    std::int32_t value;
    std::int32_t max;
};
AYLO_ECS_COMPONENT(Health, "test.type_traits.Health", value, max);

struct Inventory {
    std::vector<std::int32_t> items;
};
AYLO_ECS_COMPONENT(Inventory, "test.type_traits.Inventory");

struct Dead {};
AYLO_ECS_COMPONENT(Dead, "test.type_traits.Dead");
}  // namespace aylo::test::type_traits

namespace traits = aylo::test::type_traits;

static_assert(aylo::ecs::Described<traits::Health>);
static_assert(!aylo::ecs::Described<int>);
static_assert(aylo::ecs::TypeTraits<traits::Health>::kKind == aylo::ecs::TypeKind::kComponent);
static_assert(aylo::ecs::ComponentTraits<traits::Health>::kName == "test.type_traits.Health",
              "ComponentTraits inherits what TypeTraits knows");

TEST(TypeTraits, DescribesSizeAlignmentAndCopyability) {
    EXPECT_EQ(aylo::ecs::TypeTraits<traits::Health>::kSize, sizeof(traits::Health));
    EXPECT_EQ(aylo::ecs::TypeTraits<traits::Health>::kAlignment, alignof(traits::Health));
    EXPECT_TRUE(aylo::ecs::TypeTraits<traits::Health>::kCopyable);
    EXPECT_EQ(aylo::ecs::TypeTraits<traits::Dead>::kSize, 0U);  // an empty type is a tag
}

TEST(TypeTraits, MakesTheTypeInfo) {
    const aylo::ecs::TypeInfo info = aylo::ecs::TypeTraits<traits::Health>::makeTypeInfo();
    EXPECT_EQ(info.name, "test.type_traits.Health");
    EXPECT_EQ(info.size, sizeof(traits::Health));
    EXPECT_TRUE(info.copyable);
    ASSERT_EQ(info.fields.size(), 2U);
    EXPECT_EQ(info.fields[1].name, "max");
    EXPECT_EQ(info.fields[1].type, aylo::ecs::FieldType::kI32);
    EXPECT_EQ(aylo::ecs::computeLayoutHash(info), aylo::ecs::TypeTraits<traits::Health>::kLayoutHash);
}

TEST(TypeTraits, AComponentInfoWrapsTheTypeInfo) {
    const aylo::ecs::ComponentInfo component = aylo::ecs::ComponentTraits<traits::Inventory>::makeInfo();
    const aylo::ecs::TypeInfo type = aylo::ecs::TypeTraits<traits::Inventory>::makeTypeInfo();
    EXPECT_EQ(component.type.name, type.name);
    EXPECT_EQ(component.type.size, type.size);
    EXPECT_EQ(aylo::ecs::computeLayoutHash(component.type), aylo::ecs::computeLayoutHash(type));
    EXPECT_NE(component.type.hooks.destruct, nullptr);  // the std::vector member needs destroying
}
