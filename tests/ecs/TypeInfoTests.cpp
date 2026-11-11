/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TypeInfoTests
*/

#include <gtest/gtest.h>

#include <string>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace {
/// @return A valid description of an 8-byte type with two fields.
aylo::ecs::TypeInfo makeScore() {
    return aylo::ecs::TypeInfo{.name = "game.Score",
                               .size = 8,
                               .alignment = 4,
                               .fields = {{.name = "value", .type = aylo::ecs::FieldType::kI32, .offset = 0},
                                          {.name = "combo", .type = aylo::ecs::FieldType::kI32, .offset = 4}}};
}
}  // namespace

TEST(TypeInfo, DefaultsAreCopyableNotReplicatedOnBothSides) {
    const aylo::ecs::TypeInfo info{};
    EXPECT_EQ(info.id, aylo::ecs::kInvalidTypeId);
    EXPECT_TRUE(info.copyable);
    EXPECT_EQ(info.presence, aylo::ecs::Presence::kBoth);
    EXPECT_EQ(info.replication, aylo::ecs::ReplicationMode::kNone);
}

TEST(TypeInfo, AcceptsAValidTypeOfAnyKind) {
    EXPECT_TRUE(aylo::ecs::validateType(makeScore(), "resource"));
    aylo::ecs::TypeInfo moveOnly = makeScore();
    moveOnly.copyable = false;  // only components require copyability
    EXPECT_TRUE(aylo::ecs::validateType(moveOnly, "resource"));
}

TEST(TypeInfo, MessagesNameTheKind) {
    aylo::ecs::TypeInfo info = makeScore();
    info.alignment = 3;
    const aylo::ecs::Result<void> result = aylo::ecs::validateType(info, "resource");
    ASSERT_FALSE(result);
    const std::string message{result.error().getMessage()};
    EXPECT_NE(message.find("Resource 'game.Score'"), std::string::npos) << message;
}

TEST(TypeInfo, MessagesNameTheKindWhenTheNameIsMissing) {
    const aylo::ecs::Result<void> result = aylo::ecs::validateType(aylo::ecs::TypeInfo{}, "resource");
    ASSERT_FALSE(result);
    EXPECT_NE(std::string{result.error().getMessage()}.find("A resource needs a name"), std::string::npos);
}

TEST(TypeInfo, RejectsACopyableTypeWithoutCopyHook) {
    aylo::ecs::TypeInfo info = makeScore();
    info.hooks.destruct = [](void* /*target*/) noexcept {};  // owns memory: memcpy would not copy it
    const aylo::ecs::Result<void> result = aylo::ecs::validateType(info, "resource");
    ASSERT_FALSE(result);
    EXPECT_NE(std::string{result.error().getMessage()}.find("no copy hook"), std::string::npos);
    info.copyable = false;  // the same type, declared move-only, is fine
    EXPECT_TRUE(aylo::ecs::validateType(info, "resource"));
}
