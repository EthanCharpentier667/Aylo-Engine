/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MutTests
*/

#include <gtest/gtest.h>

#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/change/Tick.hpp"

namespace {
struct Health {
    int value;
};
}  // namespace

TEST(Mut, ReadingDoesNotMarkChanged) {
    Health health{.value = 3};
    aylo::ecs::Tick changed = 1;
    const aylo::ecs::Mut<Health> mut{&health, &changed, 9};
    EXPECT_EQ(mut.get().value, 3);
    EXPECT_EQ(changed, 1U);
}

TEST(Mut, ArrowMarksChangedAndWrites) {
    Health health{.value = 3};
    aylo::ecs::Tick changed = 1;
    aylo::ecs::Mut<Health> mut{&health, &changed, 9};
    mut->value = 4;
    EXPECT_EQ(health.value, 4);
    EXPECT_EQ(changed, 9U);
}

TEST(Mut, DereferenceMarksChangedAndWrites) {
    Health health{.value = 3};
    aylo::ecs::Tick changed = 1;
    aylo::ecs::Mut<Health> mut{&health, &changed, 9};
    (*mut).value = 5;
    EXPECT_EQ(health.value, 5);
    EXPECT_EQ(changed, 9U);
}

TEST(Mut, ArrowMarksEvenWhenOnlyReading) {
    Health health{.value = 3};
    aylo::ecs::Tick changed = 1;
    aylo::ecs::Mut<Health> mut{&health, &changed, 9};
    const int value = mut->value;  // reads, but through write access: use get() to avoid this
    EXPECT_EQ(value, 3);
    EXPECT_EQ(changed, 9U);
}

TEST(Mut, ADefaultHandleIsEmpty) {
    const aylo::ecs::Mut<Health> mut;
    EXPECT_FALSE(mut);
    Health health{.value = 0};
    aylo::ecs::Tick changed = 0;
    EXPECT_TRUE((aylo::ecs::Mut<Health>{&health, &changed, 1}));
}

TEST(Mut, CopiesShareTheValue) {
    Health health{.value = 3};
    aylo::ecs::Tick changed = 1;
    const aylo::ecs::Mut<Health> original{&health, &changed, 9};
    aylo::ecs::Mut<Health> copy = original;
    copy->value = 8;
    EXPECT_EQ(original.get().value, 8);
}
