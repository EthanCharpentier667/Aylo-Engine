/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <utility>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/exceptions/ResourceExceptions.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/ResourceRegistry.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/hash/NameHash.hpp"

namespace aylo::test::resource_registry {
struct Time {
    float delta;
    float fixedDelta;
};
AYLO_ECS_RESOURCE(Time, "test.registry.Time");

struct Backend {
    std::unique_ptr<std::int32_t> window;
};
AYLO_ECS_RESOURCE(Backend, "test.registry.Backend");

/// @brief Another layout under Time's name.
struct OtherTime {
    double delta;
};
AYLO_ECS_RESOURCE(OtherTime, "test.registry.Time");
}  // namespace aylo::test::resource_registry

namespace reg = aylo::test::resource_registry;

namespace {
/// @return A run-time description, as a Luau script would provide it.
aylo::ecs::ResourceInfo makeScore() {
    return aylo::ecs::ResourceInfo{.type = {.name = "test.registry.Score", .size = 4, .alignment = 4}};
}
}  // namespace

TEST(ResourceRegistry, GivesDenseIdsFromZero) {
    aylo::ecs::ResourceRegistry registry;
    EXPECT_EQ(registry.registerType<reg::Time>(), 0U);
    EXPECT_EQ(registry.registerType<reg::Backend>(), 1U);
    EXPECT_EQ(registry.getCount(), 2U);
}

TEST(ResourceRegistry, RegisteringTheSameResourceTwiceReturnsTheSameId) {
    aylo::ecs::ResourceRegistry registry;
    const aylo::ecs::ResourceId id = registry.registerType<reg::Time>();
    EXPECT_EQ(registry.registerType<reg::Time>(), id);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ResourceRegistry, FindsResourcesByIdNameHashAndType) {
    aylo::ecs::ResourceRegistry registry;
    const aylo::ecs::ResourceId id = registry.registerType<reg::Time>();
    const aylo::ecs::ResourceInfo* info = registry.findById(id);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->type.id, id);
    EXPECT_EQ(info->type.name, "test.registry.Time");
    EXPECT_EQ(registry.findByName("test.registry.Time"), info);
    EXPECT_EQ(registry.findByHash(aylo::core::hash::hashName("test.registry.Time")), info);
    EXPECT_EQ(registry.find<reg::Time>(), info);
}

TEST(ResourceRegistry, ReturnsNullForUnknownResources) {
    const aylo::ecs::ResourceRegistry registry;
    EXPECT_EQ(registry.findById(0), nullptr);
    EXPECT_EQ(registry.findByName("test.registry.Time"), nullptr);
    EXPECT_EQ(registry.findByHash(aylo::core::hash::hashName("test.registry.Time")), nullptr);
    EXPECT_EQ(registry.find<reg::Time>(), nullptr);
}

TEST(ResourceRegistry, AppliesTheMainThreadOption) {
    aylo::ecs::ResourceRegistry registry;
    registry.registerType<reg::Backend>({.nonSend = true});
    registry.registerType<reg::Time>();
    EXPECT_TRUE(registry.find<reg::Backend>()->nonSend);
    EXPECT_FALSE(registry.find<reg::Time>()->nonSend);
}

TEST(ResourceRegistry, RegistersRunTimeDescriptions) {
    aylo::ecs::ResourceRegistry registry;
    const aylo::ecs::Result<aylo::ecs::ResourceId> id = registry.registerType(makeScore());
    ASSERT_TRUE(id);
    EXPECT_EQ(registry.findByName("test.registry.Score")->type.id, *id);
    const aylo::ecs::Result<aylo::ecs::ResourceId> again = registry.registerType(makeScore());
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *id);
}

TEST(ResourceRegistry, FindAcceptsARunTimeDescriptionOfTheSameLayout) {
    aylo::ecs::ResourceRegistry registry;
    aylo::ecs::ResourceInfo info{.type = {.name = "test.registry.Time", .size = sizeof(reg::Time), .alignment = 4}};
    ASSERT_TRUE(registry.registerType(std::move(info)));
    EXPECT_NE(registry.find<reg::Time>(), nullptr);
}

TEST(ResourceRegistry, FindReturnsNullForARunTimeResourceThatOnlySharesTheName) {
    aylo::ecs::ResourceRegistry registry;
    ASSERT_TRUE(registry.registerType(aylo::ecs::ResourceInfo{.type = {.name = "test.registry.Time", .size = 4}}));
    EXPECT_EQ(registry.find<reg::Time>(), nullptr);
}

TEST(ResourceRegistry, RefusesInvalidDescriptions) {
    aylo::ecs::ResourceRegistry registry;
    const auto refused = [&registry](aylo::ecs::ResourceInfo info) {
        const aylo::ecs::Result<aylo::ecs::ResourceId> id = registry.registerType(std::move(info));
        return !id && id.error().getKind() == aylo::ecs::ErrorKind::kInvalidType;
    };
    EXPECT_TRUE(refused(aylo::ecs::ResourceInfo{.type = {.name = "", .size = 4, .alignment = 4}}));
    EXPECT_TRUE(refused(aylo::ecs::ResourceInfo{.type = {.name = "test.registry.A", .size = 4, .alignment = 3}}));
    EXPECT_TRUE(refused(aylo::ecs::ResourceInfo{.type = {.name = "test.registry.B", .size = 4, .alignment = 0}}));
    EXPECT_TRUE(refused(aylo::ecs::ResourceInfo{.type = {.name = "test.registry.C", .size = 6, .alignment = 4}}));
    aylo::ecs::ResourceInfo noCopy = aylo::ecs::ResourceTraits<reg::Backend>::makeInfo();
    noCopy.type.name = "test.registry.D";
    noCopy.type.copyable = true;
    EXPECT_TRUE(refused(std::move(noCopy)));
    EXPECT_EQ(registry.getCount(), 0U);
}

TEST(ResourceRegistry, RefusesTheSameNameWithAnotherLayout) {
    aylo::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    const aylo::ecs::Result<aylo::ecs::ResourceId> id =
        registry.registerType(aylo::ecs::ResourceInfo{.type = {.name = "test.registry.Time", .size = 4}});
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kLayoutMismatch);
}

TEST(ResourceRegistry, RefusesTheOtherMainThreadFlag) {
    aylo::ecs::ResourceRegistry registry;
    aylo::ecs::ResourceInfo info = makeScore();
    ASSERT_TRUE(registry.registerType(info));
    info.nonSend = true;
    const aylo::ecs::Result<aylo::ecs::ResourceId> id = registry.registerType(std::move(info));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kOptionsMismatch);
}

TEST(ResourceRegistry, TypedRegistrationThrowsOnConflicts) {
    aylo::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    EXPECT_THROW(registry.registerType<reg::OtherTime>(), aylo::ecs::exceptions::ResourceRegistrationException);
    EXPECT_THROW(registry.registerType<reg::Time>({.nonSend = true}),
                 aylo::ecs::exceptions::ResourceRegistrationException);
}

TEST(ResourceRegistry, PointersStayValidAfterManyRegistrations) {
    aylo::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    const aylo::ecs::ResourceInfo* time = registry.find<reg::Time>();
    for (int index = 0; index < 100; ++index) {
        ASSERT_TRUE(registry.registerType(aylo::ecs::ResourceInfo{
            .type = {.name = std::format("test.registry.Many{}", index), .size = 4, .alignment = 4}}));
    }
    EXPECT_EQ(registry.find<reg::Time>(), time);
}

TEST(ResourceRegistry, MessagesNameTheResource) {
    aylo::ecs::ResourceRegistry registry;
    aylo::ecs::ResourceInfo info = makeScore();
    ASSERT_TRUE(registry.registerType(info));
    info.nonSend = true;
    const aylo::ecs::Result<aylo::ecs::ResourceId> id = registry.registerType(std::move(info));
    ASSERT_FALSE(id);
    EXPECT_EQ(std::string{id.error().getMessage()},
              "Resource 'test.registry.Score' is already registered with thread 'any', not 'main'");
}

TEST(ResourceRegistry, KeepsItsResourcesWhenMoved) {
    aylo::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    const aylo::ecs::ResourceRegistry moved{std::move(registry)};
    EXPECT_NE(moved.find<reg::Time>(), nullptr);
    EXPECT_EQ(moved.getCount(), 1U);
}
