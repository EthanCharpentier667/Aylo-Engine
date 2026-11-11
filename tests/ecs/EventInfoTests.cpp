/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventInfoTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::test::event_info {
struct Hit {
    std::int32_t damage;
};
AYLO_ECS_EVENT(Hit, "test.info.Hit", damage);

struct Packet {
    std::unique_ptr<std::int32_t> payload;
};
AYLO_ECS_EVENT(Packet, "test.info.Packet");
}  // namespace aylo::test::event_info

namespace evi = aylo::test::event_info;

TEST(EventInfo, AcceptsAValidDescription) {
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::EventTraits<evi::Hit>::makeInfo()));
}

TEST(EventInfo, AcceptsAMoveOnlyEvent) {
    EXPECT_TRUE(aylo::ecs::validate(aylo::ecs::EventTraits<evi::Packet>::makeInfo()));
}

TEST(EventInfo, RefusesAReplicatedEvent) {
    aylo::ecs::EventInfo info = aylo::ecs::EventTraits<evi::Hit>::makeInfo();
    info.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    const aylo::ecs::Result<void> valid = aylo::ecs::validate(info);
    ASSERT_FALSE(valid);
    EXPECT_EQ(valid.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
}

TEST(EventInfo, AppliesTheRulesOfEveryType) {
    const aylo::ecs::Result<void> valid =
        aylo::ecs::validate(aylo::ecs::EventInfo{.type = {.name = "", .size = 4, .alignment = 4}});
    ASSERT_FALSE(valid);
    EXPECT_EQ(valid.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
}
