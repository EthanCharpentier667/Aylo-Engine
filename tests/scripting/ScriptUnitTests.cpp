/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptUnitTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "ScriptProbe.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "aylo/scripting/ScriptHost.hpp"

namespace aylo::test::units {
/// @brief A size and its unit, stored as two floats: unit 0 "auto", 1 pixels (a bare number), 2 percent.
struct Span {
    float value = 0.0F;
    float unit = 0.0F;
};
}  // namespace aylo::test::units

template <>
struct aylo::ecs::FieldTypeOf<aylo::test::units::Span>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec2> {};
template <>
struct aylo::ecs::FieldUnits<aylo::test::units::Span> {
    static constexpr std::array<aylo::ecs::FieldUnit, 3> kUnits{
        {{.name = "auto", .keyword = true}, {.name = "px"}, {.name = "%"}}};
    static constexpr std::uint8_t kBareNumber = 1;
};

namespace aylo::test::units {
/// @brief A C++ component with two fields with units and a plain vec2-sized one.
struct Box {
    Span width;
    Span left;
};
AYLO_ECS_COMPONENT(Box, "test.units.Box", width, left);
}  // namespace aylo::test::units

namespace {
using aylo::test::units::Box;
using aylo::test::units::Span;

/// @brief An App whose every frame lasts one second, and a host on it with the probe.
struct Fixture {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        app.getPluginRegistry().addScriptForms("test.Probe", aylo::tests::kProbeForms);
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("units-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @return A new entity with a Box.
    aylo::ecs::Entity spawn(Box box) {
        const aylo::ecs::Entity entity = app.getWorld().spawn();
        app.getWorld().getComponents().add(entity, box);
        return entity;
    }

    /// @return The Box of @p entity.
    [[nodiscard]] const Box& box(const aylo::ecs::Entity entity) const {
        return *app.getWorld().getComponents().get<Box>(entity);
    }
};

/// @return Whether @p span holds @p value in unit @p unit.
bool holds(const Span& span, float value, float unit) { return span.value == value && span.unit == unit; }
}  // namespace

TEST(ScriptUnits, TheFieldKnowsItsUnits) {
    const aylo::ecs::TypeInfo info = aylo::ecs::TypeTraits<Box>::makeTypeInfo();

    ASSERT_EQ(info.fields.at(0).units.size(), 3U);
    EXPECT_EQ(info.fields.at(0).units[0].name, "auto");
    EXPECT_TRUE(info.fields.at(0).units[0].keyword);
    EXPECT_EQ(info.fields.at(0).units[2].name, "%");
    EXPECT_EQ(info.fields.at(0).bareUnit, 1U);
}

TEST(ScriptUnits, AViewReadsANumberAPercentageOrAKeyword) {
    Fixture fixture;
    fixture.spawn(Box{.width = Span{.value = 50.0F, .unit = 2.0F}, .left = Span{.value = 120.0F, .unit = 1.0F}});
    fixture.spawn(Box{.width = Span{.value = 0.0F, .unit = 0.0F}, .left = Span{.value = 12.5F, .unit = 2.0F}});
    ASSERT_EQ(fixture.run(R"(
        probe.seen = {}
        engine.ecs.system("game.ReadBox", {
            phase = "Update",
            query = { read = { "test.units.Box" } },
            run = function(ctx)
                for _, box in ctx.query do
                    table.insert(probe.seen, { width = box.width, left = box.left })
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(
        local a, b = probe.seen[1], probe.seen[2]
        assert(a.width == "50%", tostring(a.width))
        assert(a.left == 120, tostring(a.left))
        assert(b.width == "auto", tostring(b.width))
        assert(b.left == "12.5%", tostring(b.left))
    )"),
              "");
}

TEST(ScriptUnits, AViewTakesWhatAStyleTakesAndTheStoredTable) {
    Fixture fixture;
    const aylo::ecs::Entity entity = fixture.spawn(Box{});
    ASSERT_EQ(fixture.run(R"(
        local frame = 0
        engine.ecs.system("game.WriteBox", {
            phase = "Update",
            query = { write = { "test.units.Box" } },
            run = function(ctx)
                frame += 1
                for _, box in ctx.query do
                    if frame == 1 then
                        box.width = 120
                        box.left = "10%"
                    elseif frame == 2 then
                        box.width = "auto"
                        box.left = "8px"
                    else
                        box.width = { x = 3, y = 2 } -- the stored form still works
                        box.left = -4
                    end
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    EXPECT_TRUE(holds(fixture.box(entity).width, 120.0F, 1.0F));
    EXPECT_TRUE(holds(fixture.box(entity).left, 10.0F, 2.0F));
    fixture.app.runFrame();
    EXPECT_TRUE(holds(fixture.box(entity).width, 0.0F, 0.0F));
    EXPECT_TRUE(holds(fixture.box(entity).left, 8.0F, 1.0F));
    fixture.app.runFrame();
    EXPECT_TRUE(holds(fixture.box(entity).width, 3.0F, 2.0F));
    EXPECT_TRUE(holds(fixture.box(entity).left, -4.0F, 1.0F));
}

TEST(ScriptUnits, AValueWithNoUnitItKnowsIsRefusedWithWhatItTakes) {
    Fixture fixture;
    const aylo::ecs::Entity entity = fixture.spawn(Box{.width = Span{.value = 7.0F, .unit = 1.0F}, .left = {}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.BadBox", {
            phase = "Update",
            query = { write = { "test.units.Box" } },
            run = function(ctx)
                for _, box in ctx.query do
                    probe.ok, probe.message = pcall(function() box.width = "tall" end)
                    probe.spaced = pcall(function() box.width = "50 %" end)
                    probe.boolOk = pcall(function() box.width = true end)
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(
        assert(not probe.ok and not probe.spaced and not probe.boolOk)
        assert(string.find(probe.message, "'tall' is not a number, \"auto\", \"<number>px\" or \"<number>%\"", 1, true),
            probe.message)
    )"),
              "");
    EXPECT_TRUE(holds(fixture.box(entity).width, 7.0F, 1.0F));
}

TEST(ScriptUnits, SpawnTakesWhatAStyleTakes) {
    Fixture fixture;
    fixture.app.getWorld().getComponents().registerType<Box>();
    ASSERT_EQ(fixture.run(R"(
        local spawned = false
        engine.ecs.system("game.SpawnBox", {
            phase = "Update",
            run = function(ctx)
                if not spawned then
                    spawned = true
                    ctx.commands:spawn({ ["test.units.Box"] = { width = "25%", left = 30 } })
                end
            end,
        })
        engine.ecs.system("game.SeeBox", {
            phase = "PostUpdate",
            query = { read = { "test.units.Box" } },
            run = function(ctx)
                for _, box in ctx.query do
                    probe.width = box.width
                    probe.left = box.left
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(assert(probe.width == "25%" and probe.left == 30, tostring(probe.width)))"), "");
}
