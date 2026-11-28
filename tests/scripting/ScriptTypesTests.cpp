/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptTypesTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "aylo/scripting/types/TypeDefinitions.hpp"

namespace {
/// @brief A host whose game declared a component, an event and a Behaviour.
struct Fixture {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Fixture() {
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        const auto ran = host->run("game.luau", R"(
            engine.ecs.component("game.Door", { fields = { open = "bool", ["end"] = "vec2", target = "entity" } })
            engine.ecs.event("game.Hit", { fields = { target = "entity", damage = "u32" } })
            local Spinner = engine.behaviour("game.Spinner", { fields = { speed = "f32" } })
            function Spinner:onUpdate() end
        )");
        EXPECT_TRUE(ran) << (ran ? "" : std::string{ran.error().message()});
    }
};

[[nodiscard]] testing::AssertionResult has(const std::string& text, const std::string_view part) {
    if (text.contains(part)) {
        return testing::AssertionSuccess();
    }
    return testing::AssertionFailure() << "no '" << part << "' in the definitions";
}
}  // namespace

TEST(ScriptTypes, DescribeTheComponentsEventsAndCallsOfTheGame) {
    const Fixture fixture;
    const std::string types = fixture.host->generateTypes();

    EXPECT_TRUE(has(types, "declare class Entity"));
    EXPECT_TRUE(has(types, R"(type C_game_Door = { target: Entity, ["end"]: Vec2, open: boolean })"))
        << "a keyword is quoted; the fields in the order of the layout";
    EXPECT_TRUE(has(types, R"((self: any, entity: Entity, name: "game.Door") -> C_game_Door?)"));
    EXPECT_TRUE(has(types, R"((self: any, name: "game.Spinner") -> C_game_Spinner?)"));
    EXPECT_TRUE(has(types, "type E_game_Hit = { target: Entity, damage: number }"));
    EXPECT_TRUE(has(types, R"((name: "game.Hit", fields: { target: (Entity)?, damage: (number)? }?) -> ())"));
    EXPECT_TRUE(has(types, "type R_aylo_state_get = string"));
    EXPECT_TRUE(has(types, "type R_aylo_state_set = nil"));
    EXPECT_TRUE(has(types, R"((name: "aylo.state.set", args: { state: (string)?, value: (string)? }?) -> ())"));
    EXPECT_TRUE(has(types, "\t(self: any, name: string) -> any)") || has(types, "& ((self: any, name: string) -> any)"))
        << "the overload of any name comes last";
    EXPECT_TRUE(has(types, "declare engine: {"));
    EXPECT_TRUE(has(types, "\tbehaviour: (name: string, spec:"));
    EXPECT_TRUE(has(types, "\tstate: {\n\t\tget: (ctx: CtxLike, machine: string) -> string,"));
    EXPECT_FALSE(has(types, "\tforms: ")) << "the helpers of the forms are not for games";
    EXPECT_EQ(types, fixture.host->generateTypes()) << "the same game gives the same text";
}

TEST(ScriptTypes, AFieldReadsAsItsLuauType) {
    using aylo::ecs::FieldInfo;
    using aylo::ecs::FieldType;
    using aylo::scripting::types::readTypeOf;
    EXPECT_EQ(readTypeOf(FieldInfo{.name = "a", .type = FieldType::kF32}), "number");
    EXPECT_EQ(readTypeOf(FieldInfo{.name = "a", .type = FieldType::kVec3}), "vector");
    EXPECT_EQ(readTypeOf(FieldInfo{.name = "a", .type = FieldType::kAssetId}), "AssetId");
    EXPECT_EQ(readTypeOf(FieldInfo{.name = "a", .type = FieldType::kU8, .count = 4}), "{ number }");
    constexpr std::string_view kNames[] = {"rect", "circle"};
    EXPECT_EQ(readTypeOf(FieldInfo{.name = "a", .type = FieldType::kU8, .enumNames = kNames}), R"("rect" | "circle")");
    EXPECT_EQ(aylo::scripting::types::typeNameOf("C_", "aylo.Transform2D"), "C_aylo_Transform2D");
}
