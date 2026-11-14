/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptEnumTests
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

namespace aylo::test::enums {
/// @brief How a walker moves: an enum whose values have names.
enum class Gait : std::uint8_t { kWalk, kRun, kFly };
}  // namespace aylo::test::enums

template <>
struct aylo::ecs::FieldTypeOf<aylo::test::enums::Gait>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
template <>
struct aylo::ecs::FieldEnumNames<aylo::test::enums::Gait> {
    static constexpr std::array<std::string_view, 3> kNames{"walk", "run", "fly"};
};

namespace aylo::test::enums {
/// @brief A C++ component with an enum field and a list of them.
struct Walker {
    Gait gait{Gait::kRun};
    std::vector<Gait> plan;
};
AYLO_ECS_COMPONENT(Walker, "test.enums.Walker", gait, plan);
}  // namespace aylo::test::enums

namespace {
using aylo::test::enums::Gait;
using aylo::test::enums::Walker;

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
        const auto result = host->run("enums-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @return A new entity with a Walker.
    aylo::ecs::Entity spawn(Walker walker) {
        const aylo::ecs::Entity entity = app.getWorld().spawn();
        app.getWorld().getComponents().add(entity, std::move(walker));
        return entity;
    }

    /// @return The Walker of @p entity.
    [[nodiscard]] const Walker& walker(const aylo::ecs::Entity entity) const {
        return *app.getWorld().getComponents().get<Walker>(entity);
    }
};
}  // namespace

TEST(ScriptEnums, TheFieldKnowsTheNamesOfItsValues) {
    const aylo::ecs::TypeInfo info = aylo::ecs::TypeTraits<Walker>::makeTypeInfo();

    ASSERT_EQ(info.fields.at(0).enumNames.size(), 3U);
    EXPECT_EQ(info.fields.at(0).enumNames[2], "fly");
    EXPECT_EQ(info.fields.at(1).enumNames.size(), 3U);  // the list's values too
}

TEST(ScriptEnums, AViewReadsTheNameOfTheValue) {
    Fixture fixture;
    fixture.spawn(Walker{.gait = Gait::kFly, .plan = {Gait::kWalk, Gait::kFly}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ReadGait", {
            phase = "Update",
            query = { read = { "test.enums.Walker" } },
            run = function(ctx)
                for _, walker in ctx.query do
                    probe.gait = walker.gait
                    probe.plan = walker.plan
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(
        assert(probe.gait == "fly", tostring(probe.gait))
        assert(probe.plan[1] == "walk" and probe.plan[2] == "fly", tostring(probe.plan[1]))
    )"),
              "");
}

TEST(ScriptEnums, AViewTakesANameOrANumber) {
    Fixture fixture;
    const aylo::ecs::Entity named = fixture.spawn(Walker{.gait = Gait::kRun, .plan = {}});
    const aylo::ecs::Entity numbered = fixture.spawn(Walker{.gait = Gait::kRun, .plan = {}});
    const auto namedBits = std::to_string(named.getBits());
    ASSERT_EQ(fixture.run(std::string{R"(
        engine.ecs.system("game.WriteGait", {
            phase = "Update",
            query = { write = { "test.enums.Walker" } },
            run = function(ctx)
                for entity, walker in ctx.query do
                    if entity.bits == )"} +
                          namedBits + R"( then
                        walker.gait = "walk"
                        walker.plan = { "fly", "run" }
                    else
                        walker.gait = 2
                    end
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.walker(named).gait, Gait::kWalk);
    EXPECT_EQ(fixture.walker(named).plan, (std::vector<Gait>{Gait::kFly, Gait::kRun}));
    EXPECT_EQ(fixture.walker(numbered).gait, Gait::kFly);
}

TEST(ScriptEnums, AnUnknownNameIsRefusedWithTheNamesItTakes) {
    Fixture fixture;
    const aylo::ecs::Entity entity = fixture.spawn(Walker{.gait = Gait::kRun, .plan = {}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.BadGait", {
            phase = "Update",
            query = { write = { "test.enums.Walker" } },
            run = function(ctx)
                for _, walker in ctx.query do
                    probe.ok, probe.message = pcall(function() walker.gait = "sprint" end)
                    probe.boolOk = pcall(function() walker.gait = true end)
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(
        assert(not probe.ok and not probe.boolOk)
        assert(string.find(probe.message, "'sprint' is not one of walk, run, fly", 1, true), probe.message)
    )"),
              "");
    EXPECT_EQ(fixture.walker(entity).gait, Gait::kRun);
}

TEST(ScriptEnums, SpawnTakesTheName) {
    Fixture fixture;
    fixture.app.getWorld().getComponents().registerType<Walker>();
    ASSERT_EQ(fixture.run(R"(
        local spawned = false
        engine.ecs.system("game.SpawnFlyer", {
            phase = "Update",
            run = function(ctx)
                if not spawned then
                    spawned = true
                    ctx.commands:spawn({ ["test.enums.Walker"] = { gait = "fly" } })
                end
            end,
        })
        engine.ecs.system("game.SeeFlyer", {
            phase = "PostUpdate",
            query = { read = { "test.enums.Walker" } },
            run = function(ctx)
                for _, walker in ctx.query do
                    probe.spawned = walker.gait
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(assert(probe.spawned == "fly", tostring(probe.spawned)))"), "");
}
