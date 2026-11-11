/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceInfoTests
*/

#include <gtest/gtest.h>

#include <string>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/type/Field.hpp"

namespace {
/// @return A valid run-time description, as a Luau script would declare a resource.
aylo::ecs::ResourceInfo makeScore() {
    return aylo::ecs::ResourceInfo{
        .type = {.name = "game.Score",
                 .size = 4,
                 .alignment = 4,
                 .replication = aylo::ecs::ReplicationMode::kEveryChange,
                 .fields = {{.name = "value", .type = aylo::ecs::FieldType::kI32, .offset = 0}}}};
}
}  // namespace

TEST(ResourceInfo, DefaultsAreUsableFromAnyThread) {
    const aylo::ecs::ResourceInfo info{};
    EXPECT_EQ(info.type.id, aylo::ecs::kInvalidResourceId);
    EXPECT_FALSE(info.nonSend);
}

TEST(ResourceInfo, AcceptsAReplicatedResourceWithFields) { EXPECT_TRUE(aylo::ecs::validate(makeScore())); }

TEST(ResourceInfo, AcceptsAMoveOnlyResource) {
    aylo::ecs::ResourceInfo backend{.type = {.name = "aylo.Backend", .size = 8, .alignment = 8, .copyable = false},
                                    .nonSend = true};
    EXPECT_TRUE(aylo::ecs::validate(backend));
}

TEST(ResourceInfo, RejectsAnInvalidDescriptionNamingTheResource) {
    aylo::ecs::ResourceInfo info = makeScore();
    info.type.fields.clear();  // replicated, but nothing to serialize
    const aylo::ecs::Result<void> result = aylo::ecs::validate(info);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
    const std::string message{result.error().getMessage()};
    EXPECT_NE(message.find("Resource 'game.Score'"), std::string::npos) << message;
    EXPECT_NE(message.find("a replicated resource needs fields"), std::string::npos) << message;
}
