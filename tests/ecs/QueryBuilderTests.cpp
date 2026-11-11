/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryBuilderTests
*/

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/query/DynamicQuery.hpp"
#include "aylo/ecs/query/QueryBuilder.hpp"
#include "aylo/ecs/query/QueryTerm.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::query_builder {
struct Position {
    float x;
    float y;
};
AYLO_ECS_COMPONENT(Position, "test.query_builder.Position", x, y);

struct Velocity {
    float x;
    float y;
};
AYLO_ECS_COMPONENT(Velocity, "test.query_builder.Velocity", x, y);

struct Dead {};
AYLO_ECS_COMPONENT(Dead, "test.query_builder.Dead");
}  // namespace aylo::test::query_builder

namespace {
using aylo::ecs::ComponentAccess;
using aylo::ecs::ComponentId;
using aylo::ecs::DynamicQuery;
using aylo::ecs::ErrorKind;
using aylo::ecs::QueryBuilder;
using aylo::ecs::QueryTerm;
using aylo::ecs::Result;
using aylo::ecs::TermKind;
using aylo::ecs::World;

/// @brief A world with three registered components, and their ids.
class QueryBuilderTest : public ::testing::Test {
  protected:
    World world;
    aylo::ecs::Components& components{world.getComponents()};
    ComponentId position{components.registerType<aylo::test::query_builder::Position>()};
    ComponentId velocity{components.registerType<aylo::test::query_builder::Velocity>()};
    ComponentId dead{components.registerType<aylo::test::query_builder::Dead>()};
};

/// @brief Expects `query` to be refused as an invalid query whose message contains `text`.
void expectInvalid(const Result<DynamicQuery>& query, std::string_view text) {
    ASSERT_FALSE(query);
    EXPECT_EQ(query.error().getKind(), ErrorKind::kInvalidQuery);
    EXPECT_NE(query.error().getMessage().find(text), std::string_view::npos) << query.error().getMessage();
}
}  // namespace

TEST(QueryTerm, ClassifiesKinds) {
    EXPECT_TRUE(aylo::ecs::isDataTerm(TermKind::kRead));
    EXPECT_TRUE(aylo::ecs::isDataTerm(TermKind::kOptionalWrite));
    EXPECT_FALSE(aylo::ecs::isDataTerm(TermKind::kWith));
    EXPECT_FALSE(aylo::ecs::isDataTerm(TermKind::kChanged));

    EXPECT_TRUE(aylo::ecs::isRequiredTerm(TermKind::kWrite));
    EXPECT_TRUE(aylo::ecs::isRequiredTerm(TermKind::kWith));
    EXPECT_TRUE(aylo::ecs::isRequiredTerm(TermKind::kAdded));
    EXPECT_FALSE(aylo::ecs::isRequiredTerm(TermKind::kOptionalRead));
    EXPECT_FALSE(aylo::ecs::isRequiredTerm(TermKind::kWithout));

    EXPECT_TRUE(aylo::ecs::isWriteTerm(TermKind::kWrite));
    EXPECT_TRUE(aylo::ecs::isWriteTerm(TermKind::kOptionalWrite));
    EXPECT_FALSE(aylo::ecs::isWriteTerm(TermKind::kRead));
}

TEST_F(QueryBuilderTest, KeepsTermsInOrder) {
    const Result<DynamicQuery> query = QueryBuilder{}.write(position).read(velocity).without(dead).build(components);
    ASSERT_TRUE(query) << query.error().getMessage();
    const std::vector<QueryTerm> expected{{.id = position, .kind = TermKind::kWrite},
                                          {.id = velocity, .kind = TermKind::kRead},
                                          {.id = dead, .kind = TermKind::kWithout}};
    EXPECT_EQ(std::vector<QueryTerm>(query->getTerms().begin(), query->getTerms().end()), expected);
    EXPECT_EQ(query->getDataTermCount(), 2U);
}

TEST_F(QueryBuilderTest, CountsOptionalTermsAsData) {
    const Result<DynamicQuery> query =
        QueryBuilder{}.with(dead).optionalRead(position).optionalWrite(velocity).build(components);
    ASSERT_TRUE(query) << query.error().getMessage();
    EXPECT_EQ(query->getDataTermCount(), 2U);
}

TEST_F(QueryBuilderTest, ReportsAccess) {
    const Result<DynamicQuery> query =
        QueryBuilder{}.read(position).optionalWrite(velocity).with(dead).build(components);
    ASSERT_TRUE(query) << query.error().getMessage();
    const std::vector<ComponentAccess> expected{{.id = position, .write = false}, {.id = velocity, .write = true}};
    EXPECT_EQ(std::vector<ComponentAccess>(query->getAccess().begin(), query->getAccess().end()), expected);
}

TEST_F(QueryBuilderTest, CountsTickFiltersAsReads) {
    const Result<DynamicQuery> query = QueryBuilder{}.added(position).changed(velocity).build(components);
    ASSERT_TRUE(query) << query.error().getMessage();
    const std::vector<ComponentAccess> expected{{.id = position, .write = false}, {.id = velocity, .write = false}};
    EXPECT_EQ(std::vector<ComponentAccess>(query->getAccess().begin(), query->getAccess().end()), expected);
}

TEST_F(QueryBuilderTest, ListsEachComponentOnceWithWriteWinning) {
    const Result<DynamicQuery> query = QueryBuilder{}.changed(position).write(position).build(components);
    ASSERT_TRUE(query) << query.error().getMessage();
    const std::vector<ComponentAccess> expected{{.id = position, .write = true}};
    EXPECT_EQ(std::vector<ComponentAccess>(query->getAccess().begin(), query->getAccess().end()), expected);
}

TEST_F(QueryBuilderTest, AllowsFiltersOnADataTermComponent) {
    EXPECT_TRUE(QueryBuilder{}.write(position).changed(position).build(components));
    EXPECT_TRUE(QueryBuilder{}.read(position).with(position).build(components));
    EXPECT_TRUE(QueryBuilder{}.added(position).changed(position).build(components));
}

TEST_F(QueryBuilderTest, BuildsTheSameQueryTwice) {
    QueryBuilder builder;
    builder.read(position).without(dead);
    const Result<DynamicQuery> first = builder.build(components);
    const Result<DynamicQuery> second = builder.build(components);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(std::vector<QueryTerm>(first->getTerms().begin(), first->getTerms().end()),
              std::vector<QueryTerm>(second->getTerms().begin(), second->getTerms().end()));
}

TEST_F(QueryBuilderTest, RejectsAnUnknownComponent) {
    expectInvalid(QueryBuilder{}.read(position).read(42).build(components), "component id 42 is not registered");
}

TEST_F(QueryBuilderTest, RejectsAnEmptyQuery) {
    expectInvalid(QueryBuilder{}.build(components), "at least one required term");
}

TEST_F(QueryBuilderTest, RejectsOptionalTermsOnly) {
    expectInvalid(QueryBuilder{}.optionalRead(position).optionalWrite(velocity).build(components),
                  "at least one required term");
}

TEST_F(QueryBuilderTest, RejectsWithoutOnly) {
    expectInvalid(QueryBuilder{}.without(dead).build(components), "at least one required term");
}

TEST_F(QueryBuilderTest, RejectsTwoWrites) {
    expectInvalid(QueryBuilder{}.write(position).write(position).build(components),
                  "'test.query_builder.Position' has the same term twice");
}

TEST_F(QueryBuilderTest, RejectsAReadAndAWriteOfOneComponent) {
    expectInvalid(QueryBuilder{}.read(position).write(position).build(components),
                  "'test.query_builder.Position' is in two data terms");
}

TEST_F(QueryBuilderTest, RejectsARequiredAndAnOptionalTermOfOneComponent) {
    expectInvalid(QueryBuilder{}.read(position).optionalWrite(position).build(components), "in two data terms");
}

TEST_F(QueryBuilderTest, RejectsAnExcludedComponentUsedElsewhere) {
    expectInvalid(QueryBuilder{}.read(position).without(position).build(components), "excluded by without()");
    expectInvalid(QueryBuilder{}.read(velocity).with(dead).without(dead).build(components), "excluded by without()");
}

TEST_F(QueryBuilderTest, RejectsTheSameFilterTwice) {
    expectInvalid(QueryBuilder{}.read(position).changed(velocity).changed(velocity).build(components),
                  "has the same term twice");
}
