/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentInfoTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/error/Error.hpp"

namespace {
/// @return A valid description of a `{ std::int32_t value; std::int32_t max; }` component.
aylo::ecs::ComponentInfo makeHealth() {
    return aylo::ecs::ComponentInfo{
        .type = {.name = "aylo.Health",
                 .size = 8,
                 .alignment = 4,
                 .fields = {{.name = "value", .type = aylo::ecs::FieldType::kI32, .offset = 0},
                            {.name = "max", .type = aylo::ecs::FieldType::kI32, .offset = 4}}}};
}

/// @brief Expects validate() to reject the description with a message containing every given word.
void expectInvalid(const aylo::ecs::ComponentInfo& info, std::initializer_list<std::string_view> words) {
    const aylo::ecs::Result<void> result = aylo::ecs::validate(info);
    ASSERT_FALSE(result) << "expected '" << info.type.name << "' to be rejected";
    EXPECT_EQ(result.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
    const std::string message{result.error().getMessage()};
    for (const std::string_view word : words) {
        EXPECT_NE(message.find(word), std::string::npos) << "'" << word << "' missing from: " << message;
    }
}
}  // namespace

TEST(FieldLayout, MatchesTheCppTypes) {
    using aylo::ecs::FieldType;
    using aylo::ecs::getFieldLayout;
    EXPECT_EQ(getFieldLayout(FieldType::kBool).size, sizeof(bool));
    EXPECT_EQ(getFieldLayout(FieldType::kI8).size, 1U);
    EXPECT_EQ(getFieldLayout(FieldType::kU16).size, 2U);
    EXPECT_EQ(getFieldLayout(FieldType::kI32).size, 4U);
    EXPECT_EQ(getFieldLayout(FieldType::kF32).size, sizeof(float));
    EXPECT_EQ(getFieldLayout(FieldType::kF64).size, sizeof(double));
    EXPECT_EQ(getFieldLayout(FieldType::kU64).alignment, alignof(std::uint64_t));
    EXPECT_EQ(getFieldLayout(FieldType::kVec2).size, 8U);
    EXPECT_EQ(getFieldLayout(FieldType::kVec3).size, 12U);
    EXPECT_EQ(getFieldLayout(FieldType::kVec4).size, 16U);
    EXPECT_EQ(getFieldLayout(FieldType::kVec4).alignment, 4U);
    EXPECT_EQ(getFieldLayout(FieldType::kEntity).size, 8U);
    EXPECT_EQ(getFieldLayout(FieldType::kAssetId).size, 8U);
}

TEST(ComponentInfo, DefaultsAreNotReplicatedSparseSetOnBothSides) {
    const aylo::ecs::ComponentInfo info{};
    EXPECT_EQ(info.type.id, aylo::ecs::kInvalidComponentId);
    EXPECT_EQ(info.storage, aylo::ecs::StorageKind::kSparseSet);
    EXPECT_EQ(info.type.presence, aylo::ecs::Presence::kBoth);
    EXPECT_EQ(info.type.replication, aylo::ecs::ReplicationMode::kNone);
    EXPECT_EQ(info.type.hooks.construct, nullptr);
}

TEST(Validate, AcceptsAValidComponent) { EXPECT_TRUE(aylo::ecs::validate(makeHealth())); }

TEST(Validate, AcceptsATag) {
    EXPECT_TRUE(
        aylo::ecs::validate(aylo::ecs::ComponentInfo{.type = {.name = "aylo.Dead", .size = 0, .alignment = 1}}));
}

TEST(Validate, AcceptsAReplicatedComponentAndAReplicatedTag) {
    aylo::ecs::ComponentInfo health = makeHealth();
    health.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    EXPECT_TRUE(aylo::ecs::validate(health));

    const aylo::ecs::ComponentInfo tag{
        .type = {
            .name = "aylo.Player", .size = 0, .alignment = 1, .replication = aylo::ecs::ReplicationMode::kSpawnOnly}};
    EXPECT_TRUE(aylo::ecs::validate(tag));
}

TEST(Validate, AcceptsAFixedArrayField) {
    const aylo::ecs::ComponentInfo info{
        .type = {.name = "game.Path",
                 .size = 32,
                 .alignment = 4,
                 .fields = {{.name = "points", .type = aylo::ecs::FieldType::kVec2, .offset = 0, .count = 4}}}};
    EXPECT_TRUE(aylo::ecs::validate(info));
}

TEST(Validate, RejectsAnEmptyName) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.name.clear();
    expectInvalid(info, {"name"});
}

TEST(Validate, RejectsAnAlignmentThatIsNotAPowerOfTwo) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.alignment = 0;
    expectInvalid(info, {"aylo.Health", "alignment 0"});
    info.type.alignment = 3;
    expectInvalid(info, {"aylo.Health", "alignment 3"});
}

TEST(Validate, RejectsASizeThatIsNotAMultipleOfTheAlignment) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.size = 6;
    expectInvalid(info, {"aylo.Health", "size 6"});
}

TEST(Validate, RejectsATagWithFields) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.size = 0;
    expectInvalid(info, {"aylo.Health", "tag"});
}

TEST(Validate, RejectsAReplicatedClientOnlyComponent) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.presence = aylo::ecs::Presence::kClientOnly;
    info.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    expectInvalid(info, {"aylo.Health", "client-only"});
}

TEST(Validate, RejectsAReplicatedComponentWithoutFields) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.clear();
    info.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    expectInvalid(info, {"aylo.Health", "fields"});
}

TEST(Validate, RejectsAFieldWithoutName) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.at(1).name.clear();
    expectInvalid(info, {"aylo.Health", "offset 4", "no name"});
}

TEST(Validate, RejectsADuplicateField) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.at(1).name = "value";
    expectInvalid(info, {"aylo.Health", "'value'", "twice"});
}

TEST(Validate, RejectsAnEmptyArrayField) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.at(1).count = 0;
    expectInvalid(info, {"aylo.Health", "'max'", "count of 0"});
}

TEST(Validate, RejectsAMisalignedField) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.at(1).offset = 2;
    expectInvalid(info, {"aylo.Health", "'max'", "aligned"});
}

TEST(Validate, RejectsAFieldPastTheEnd) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.at(1).offset = 8;
    expectInvalid(info, {"aylo.Health", "'max'", "ends at byte 12"});
}

TEST(Validate, RejectsAHugeArrayWithoutOverflowing) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.fields.at(1).count = 0xFFFFFFFFU;
    expectInvalid(info, {"aylo.Health", "'max'", "past the component size"});
}

TEST(Validate, RejectsOverlappingFields) {
    aylo::ecs::ComponentInfo info{
        .type = {.name = "game.Shield",
                 .size = 16,
                 .alignment = 4,
                 .fields = {{.name = "strength", .type = aylo::ecs::FieldType::kF32, .offset = 4},
                            {.name = "direction", .type = aylo::ecs::FieldType::kVec2, .offset = 0}}}};
    expectInvalid(info, {"game.Shield", "'direction'", "'strength'", "overlap"});
}

TEST(LayoutHash, IsStable) {
    EXPECT_EQ(aylo::ecs::computeLayoutHash(makeHealth().type), aylo::ecs::computeLayoutHash(makeHealth().type));
}

TEST(Validate, RejectsANonCopyableComponent) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.type.copyable = false;
    expectInvalid(info, {"aylo.Health", "copyable"});
}

TEST(LayoutHash, ChangesWithTheLayout) {
    const std::uint64_t original = aylo::ecs::computeLayoutHash(makeHealth().type);

    aylo::ecs::ComponentInfo renamed = makeHealth();
    renamed.type.name = "aylo.Shield";
    EXPECT_NE(aylo::ecs::computeLayoutHash(renamed.type), original);

    aylo::ecs::ComponentInfo bigger = makeHealth();
    bigger.type.size = 16;
    EXPECT_NE(aylo::ecs::computeLayoutHash(bigger.type), original);

    aylo::ecs::ComponentInfo aligned = makeHealth();
    aligned.type.alignment = 8;
    EXPECT_NE(aylo::ecs::computeLayoutHash(aligned.type), original);

    aylo::ecs::ComponentInfo fieldRenamed = makeHealth();
    fieldRenamed.type.fields.at(1).name = "maximum";
    EXPECT_NE(aylo::ecs::computeLayoutHash(fieldRenamed.type), original);

    aylo::ecs::ComponentInfo retyped = makeHealth();
    retyped.type.fields.at(1).type = aylo::ecs::FieldType::kU32;
    EXPECT_NE(aylo::ecs::computeLayoutHash(retyped.type), original);

    aylo::ecs::ComponentInfo moved = makeHealth();
    moved.type.fields.at(1).offset = 0;
    EXPECT_NE(aylo::ecs::computeLayoutHash(moved.type), original);

    aylo::ecs::ComponentInfo array = makeHealth();
    array.type.fields.at(1).count = 2;
    EXPECT_NE(aylo::ecs::computeLayoutHash(array.type), original);

    aylo::ecs::ComponentInfo destroyed = makeHealth();
    destroyed.type.hooks.destruct = [](void* /*target*/) noexcept {};
    EXPECT_NE(aylo::ecs::computeLayoutHash(destroyed.type), original);

    aylo::ecs::ComponentInfo moveOnly = makeHealth();
    moveOnly.type.copyable = false;
    EXPECT_NE(aylo::ecs::computeLayoutHash(moveOnly.type), original);
}

TEST(LayoutHash, IgnoresTheOptions) {
    aylo::ecs::ComponentInfo info = makeHealth();
    info.storage = aylo::ecs::StorageKind::kTable;
    info.type.presence = aylo::ecs::Presence::kServerOnly;
    info.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    EXPECT_EQ(aylo::ecs::computeLayoutHash(info.type), aylo::ecs::computeLayoutHash(makeHealth().type));
}

TEST(LayoutHash, DependsOnWhichHooksExistNotOnTheirAddress) {
    aylo::ecs::ComponentInfo first = makeHealth();
    aylo::ecs::ComponentInfo second = makeHealth();
    first.type.hooks.destruct = [](void* /*target*/) noexcept {};
    second.type.hooks.destruct = [](void* target) noexcept { static_cast<void>(target); };  // another function
    EXPECT_EQ(aylo::ecs::computeLayoutHash(first.type), aylo::ecs::computeLayoutHash(second.type));
    second.type.hooks.copy = [](void* /*destination*/, const void* /*source*/) {};
    EXPECT_NE(aylo::ecs::computeLayoutHash(first.type), aylo::ecs::computeLayoutHash(second.type));
}
