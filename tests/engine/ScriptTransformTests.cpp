/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptTransformTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::Transform2D;
using aylo::plugins::transform::TransformInterpolation;

/// @brief A single-player App whose frames last half a fixed tick (10 ticks per second, 50 ms frames), with the
/// TransformPlugin and a script host on it.
struct Scripts {
    aylo::ecs::App app{aylo::ecs::RunMode::kStandalone};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Scripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{50}, aylo::ecs::FixedTimestep{10, 5}));
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("transform-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptTransform, AScriptInterpolatesWhatItMovesInFixedUpdate) {
    Scripts scripts;
    ASSERT_EQ(scripts.run(R"(
        engine.ecs.system("test.Spawn", {
            phase = "Startup",
            query = {},
            run = function(ctx)
                ctx.commands:spawn({ ["aylo.Transform2D"] = {}, ["aylo.TransformInterpolation"] = {} })
            end,
        })
        engine.ecs.system("test.Move", {
            phase = "FixedUpdate",
            query = { write = { "aylo.Transform2D" } },
            run = function(ctx)
                for entity, transform in ctx.query do
                    transform.position = { x = transform.position.x + 10, y = 0 }
                end
            end,
        })
    )"),
              "");
    for (int frame = 0; frame < 3; ++frame) {  // no tick, tick 1, then halfway to the next tick
        scripts.app.runFrame();
    }

    int seen = 0;
    const aylo::ecs::QueryState<const Transform2D, const GlobalTransform2D, const TransformInterpolation> state{
        scripts.app.getWorld().getComponents()};
    aylo::ecs::Query<const Transform2D, const GlobalTransform2D, const TransformInterpolation> ships{state, 0};
    for (auto [entity, transform, global, interpolation] : ships) {
        EXPECT_EQ(transform.position.x, 10.0F);  // the simulation is untouched...
        EXPECT_EQ(global.position.x, 5.0F);      // ...and drawn halfway between the last two ticks
        ++seen;
    }
    EXPECT_EQ(seen, 1);
}

TEST(ScriptTransform, AScriptPlacesA3DChildRotatedByTheHelpers) {
    Scripts scripts;
    ASSERT_EQ(scripts.run(R"(
        engine.ecs.system("test.Spawn3D", {
            phase = "Startup",
            run = function(ctx)
                local parent = ctx.commands:spawn({
                    ["aylo.Transform3D"] = {
                        position = vector.create(0, 5, 0),
                        rotation = engine.math.quat.fromAxisAngle(vector.create(0, 1, 0), math.pi / 2),
                    },
                })
                local child = ctx.commands:spawn({ ["aylo.Transform3D"] = { position = { x = 2, y = 0, z = 0 } } })
                ctx.commands:setParent(child, parent)
            end,
        })
    )"),
              "");

    scripts.app.runFrame();
    scripts.app.runFrame();

    using aylo::plugins::transform::GlobalTransform3D;
    using aylo::plugins::transform::Transform3D;
    const aylo::ecs::QueryState<const Transform3D, const GlobalTransform3D> state{
        scripts.app.getWorld().getComponents()};
    aylo::ecs::Query<const Transform3D, const GlobalTransform3D> placed{state, 0};
    int children = 0;
    for (auto [entity, transform, global] : placed) {
        if (transform.position.x == 2.0F) {
            // A quarter turn around Y takes +X to -Z, under a parent 5 units up.
            EXPECT_NEAR(global.position.x, 0.0F, 1e-5F);
            EXPECT_NEAR(global.position.y, 5.0F, 1e-5F);
            EXPECT_NEAR(global.position.z, -2.0F, 1e-5F);
            ++children;
        }
    }
    EXPECT_EQ(children, 1);
}

TEST(ScriptTransform, TheRotationHelpersAgreeWithEachOther) {
    Scripts scripts;
    EXPECT_EQ(scripts.run(R"(
        local quat = engine.math.quat
        local function near(a, b) return math.abs(a - b) < 1e-5 end
        local function nearVector(a, b) return near(a.x, b.x) and near(a.y, b.y) and near(a.z, b.z) end

        local id = quat.identity()
        assert(id.x == 0 and id.y == 0 and id.z == 0 and id.w == 1)

        -- fromEuler and toEuler go both ways
        local q = quat.fromEuler(0.3, -1.1, 0.5)
        local x, y, z = quat.toEuler(q)
        assert(near(x, 0.3) and near(y, -1.1) and near(z, 0.5), `{x} {y} {z}`)

        -- a quarter turn around Y takes -Z (forward) to -X (left)
        local left = quat.rotate(quat.fromEuler(0, math.pi / 2, 0), vector.create(0, 0, -1))
        assert(nearVector(left, vector.create(-1, 0, 0)), tostring(left))

        -- lookRotation turns -Z toward the direction
        local look = quat.lookRotation(vector.create(1, 0, 0))
        assert(nearVector(quat.rotate(look, vector.create(0, 0, -1)), vector.create(1, 0, 0)))

        -- multiply applies the right one first; inverse undoes
        local both = quat.multiply(quat.inverse(q), q)
        assert(near(math.abs(both.w), 1))

        -- slerp halfway between nothing and a half turn is a quarter turn
        local half = quat.slerp(id, quat.fromEuler(0, math.pi, 0), 0.5)
        local _, yaw = quat.toEuler(half)
        assert(near(yaw, math.pi / 2), tostring(yaw))
    )"),
              "");
}

TEST(ScriptTransform, TheRotationHelpersNameWhatIsWrong) {
    Scripts scripts;
    EXPECT_EQ(scripts.run(R"(
        local ok, message = pcall(engine.math.quat.rotate, { x = 0 }, vector.create(1, 0, 0))
        assert(not ok and string.find(message, "engine.math.quat: the quaternion must be a quaternion", 1, true), message)
        ok, message = pcall(engine.math.quat.fromAxisAngle, vector.create(0, 0, 0), 1)
        assert(not ok and string.find(message, "the axis must not be zero", 1, true), message)
    )"),
              "");
}

TEST(ScriptTransform, TheManualsRotationRecipesRun) {
    Scripts scripts;
    ASSERT_EQ(scripts.run(R"(
        local quat = engine.math.quat
        local target = vector.create(0, 0, 0)
        engine.ecs.component("game.Turret", { fields = {} })
        engine.ecs.component("game.Spinner", { fields = {} })
        engine.ecs.system("test.SpawnRecipes", {
            phase = "Startup",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(5, 0, 0) },
                    ["game.Turret"] = {},
                })
                ctx.commands:spawn({ ["aylo.Transform3D"] = {}, ["game.Spinner"] = {} })
            end,
        })
        engine.ecs.system("game.FaceTheCentre", {
            phase = "Update",
            query = { write = { "aylo.Transform3D" }, with = { "game.Turret" } },
            run = function(ctx)
                for _, transform in ctx.query do
                    transform.rotation = quat.lookRotation(target - transform.position)
                end
            end,
        })
        engine.ecs.system("game.Spin", {
            phase = "Update",
            query = { write = { "aylo.Transform3D" }, with = { "game.Spinner" } },
            after = { "game.FaceTheCentre" },
            run = function(ctx)
                local step = quat.fromAxisAngle(vector.create(0, 1, 0), (math.pi / 2) * ctx.time.delta)
                for _, transform in ctx.query do
                    transform.rotation = quat.normalize(quat.multiply(step, transform.rotation))
                end
            end,
        })
    )"),
              "");
    testing::internal::CaptureStderr();
    for (int frame = 0; frame < 3; ++frame) {
        scripts.app.runFrame();
    }
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(log.find("failed"), std::string::npos) << log;

    using aylo::plugins::transform::Transform3D;
    const aylo::ecs::QueryState<const Transform3D> state{scripts.app.getWorld().getComponents()};
    aylo::ecs::Query<const Transform3D> transforms{state, 0};
    for (auto [entity, transform] : transforms) {
        if (transform.position.x == 5.0F) {
            // Facing the centre from +X: a quarter turn to the left, around Y.
            EXPECT_NEAR(std::abs(transform.rotation.y), std::sin(0.25F * 3.14159265F), 1e-4F);
        }
    }
}
