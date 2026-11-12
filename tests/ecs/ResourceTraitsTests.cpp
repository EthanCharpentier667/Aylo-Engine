/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceTraitsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "core/hash/NameHash.hpp"

namespace aylo::test::resource_traits {
struct Time {
    float delta;
    float fixedDelta;
};
AYLO_ECS_RESOURCE(Time, "aylo.Time");

/// @brief A class with a constructor and private members: not a valid component, but a valid resource.
class Settings {
  public:
    explicit Settings(std::string path) : _path{std::move(path)} {}
    ~Settings() = default;
    Settings(const Settings& other) = default;
    Settings& operator=(const Settings& other) = default;
    Settings(Settings&& other) noexcept = default;
    Settings& operator=(Settings&& other) noexcept = default;

    [[nodiscard]] const std::string& getPath() const noexcept { return _path; }

  private:
    std::string _path;  ///< Path of the configuration file.
};
AYLO_ECS_RESOURCE(Settings, "aylo.Settings");

/// @brief A move-only resource, like the platform and renderer backend.
struct Backend {
    std::unique_ptr<std::int32_t> window;
};
AYLO_ECS_RESOURCE(Backend, "aylo.Backend");

struct Paused {};
AYLO_ECS_RESOURCE(Paused, "aylo.Paused");

struct alignas(16) Aligned16 {
    std::array<float, 4> values;
};
AYLO_ECS_RESOURCE(Aligned16, "test.Aligned16");

/// @brief A reflected resource: replicable and reachable from Luau.
struct Score {
    std::int32_t value;
};
AYLO_ECS_RESOURCE(Score, "game.Score", value);

struct NotDeclared {};
}  // namespace aylo::test::resource_traits

namespace traits = aylo::test::resource_traits;

TEST(ResourceTraits, ConceptOnlyAcceptsDeclaredTypes) {
    static_assert(aylo::ecs::Resource<traits::Time>);
    static_assert(aylo::ecs::Resource<traits::Settings>);
    static_assert(aylo::ecs::Resource<traits::Backend>);
    static_assert(!aylo::ecs::Resource<traits::NotDeclared>);
    static_assert(!aylo::ecs::Resource<std::int32_t>);
}

TEST(ResourceTraits, AResourceIsNotAComponent) {
    static_assert(aylo::ecs::Described<traits::Time>);
    static_assert(!aylo::ecs::Component<traits::Time>);
    static_assert(aylo::ecs::ResourceTraits<traits::Time>::kKind == aylo::ecs::TypeKind::kResource);
}

TEST(ResourceTraits, ReflectsDeclaredFields) {
    const aylo::ecs::ResourceInfo info = aylo::ecs::ResourceTraits<traits::Score>::makeInfo();
    ASSERT_EQ(info.type.fields.size(), 1U);
    EXPECT_EQ(info.type.fields[0].name, "value");
    EXPECT_EQ(info.type.fields[0].type, aylo::ecs::FieldType::kI32);
    EXPECT_TRUE(aylo::ecs::validate(info));
}

TEST(ResourceTraits, LayoutHashMatchesTheRunTimeHash) {
    EXPECT_EQ(aylo::ecs::ResourceTraits<traits::Backend>::kLayoutHash,
              aylo::ecs::computeLayoutHash(aylo::ecs::ResourceTraits<traits::Backend>::makeInfo().type));
}

TEST(ResourceTraits, NameAndHash) {
    static_assert(aylo::ecs::ResourceTraits<traits::Time>::kName == "aylo.Time");
    static_assert(aylo::ecs::ResourceTraits<traits::Time>::kHash == aylo::core::hash::hashName("aylo.Time"));
}

TEST(ResourceTraits, PlainDataHasNoHooks) {
    const aylo::ecs::ResourceInfo info = aylo::ecs::ResourceTraits<traits::Time>::makeInfo();
    EXPECT_EQ(info.type.id, aylo::ecs::kInvalidResourceId);
    EXPECT_EQ(info.type.name, "aylo.Time");
    EXPECT_EQ(info.type.size, sizeof(traits::Time));
    EXPECT_EQ(info.type.alignment, alignof(traits::Time));
    EXPECT_TRUE(info.type.copyable);
    EXPECT_FALSE(info.nonSend);
    EXPECT_EQ(info.type.hooks.construct, nullptr);
    EXPECT_EQ(info.type.hooks.destruct, nullptr);
    EXPECT_EQ(info.type.hooks.move, nullptr);
    EXPECT_EQ(info.type.hooks.copy, nullptr);
}

TEST(ResourceTraits, ClassWithConstructorHasEveryHookButConstruct) {
    const aylo::ecs::ResourceInfo info = aylo::ecs::ResourceTraits<traits::Settings>::makeInfo();
    EXPECT_TRUE(info.type.copyable);
    EXPECT_EQ(info.type.hooks.construct, nullptr);
    EXPECT_NE(info.type.hooks.destruct, nullptr);
    EXPECT_NE(info.type.hooks.move, nullptr);
    EXPECT_NE(info.type.hooks.copy, nullptr);
}

TEST(ResourceTraits, MoveOnlyTypeHasNoCopy) {
    const aylo::ecs::ResourceInfo info = aylo::ecs::ResourceTraits<traits::Backend>::makeInfo();
    EXPECT_FALSE(info.type.copyable);
    EXPECT_NE(info.type.hooks.destruct, nullptr);
    EXPECT_NE(info.type.hooks.move, nullptr);
    EXPECT_EQ(info.type.hooks.copy, nullptr);
}

TEST(ResourceTraits, EmptyTypeHasSizeZero) {
    EXPECT_EQ(aylo::ecs::ResourceTraits<traits::Paused>::makeInfo().type.size, 0U);
}

TEST(ResourceTraits, KeepsOverAlignment) {
    EXPECT_EQ(aylo::ecs::ResourceTraits<traits::Aligned16>::makeInfo().type.alignment, 16U);
}

TEST(ResourceTraits, HooksMoveAndDestroy) {
    const aylo::ecs::ResourceInfo info = aylo::ecs::ResourceTraits<traits::Settings>::makeInfo();
    traits::Settings source{"config.toml"};
    alignas(traits::Settings) std::array<std::byte, sizeof(traits::Settings)> buffer{};
    info.type.hooks.move(buffer.data(), &source);
    auto* moved = std::launder(reinterpret_cast<traits::Settings*>(buffer.data()));
    EXPECT_EQ(moved->getPath(), "config.toml");
    info.type.hooks.destruct(moved);
}
